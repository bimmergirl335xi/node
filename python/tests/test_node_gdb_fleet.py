#!/usr/bin/env python3
"""Deterministic host tests for the bounded Node GDB fleet controller."""

from __future__ import annotations

import os
import sys
import tempfile
import textwrap
import time
import unittest
from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PYTHON_ROOT))

import node_gdb_fleet as fleet  # noqa: E402
import node_lab  # noqa: E402


def profile(node_id: str, port: int) -> node_lab.NodeProfile:
    suffix = int(node_id.rsplit("-", 1)[1])
    return node_lab.NodeProfile(
        node_id=node_id,
        firmware="bios",
        vcpus=1,
        cpu_sockets=1,
        cpu_cores=1,
        cpu_threads=1,
        memory_mb=512,
        debug_enabled=False,
        gdb_port=port,
        network_enabled=True,
        mac_address=f"02:00:00:00:00:{suffix:02x}",
        ipv4_address=f"10.77.0.{suffix}/24",
        acs_port=39001,
        network_multicast_address="230.0.0.1",
        network_multicast_port=39000,
        network_local_address="127.0.0.1",
        acs_mode="explicit",
        acs_peers=(),
    )


class FakeSession:
    failing_node = ""

    def __init__(self, item, _gdb, _vmlinux, event_sink=None):
        self.profile = item
        self.commands: list[str] = []
        self.closed = False
        self.event_sink = event_sink

    def attach(self):
        return fleet.CommandResult(True, "connected")

    def execute(self, command):
        self.commands.append(command)
        if self.profile.node_id == self.failing_node:
            raise RuntimeError("isolated failure")
        return fleet.CommandResult(True, "done", (f"output:{command}\n",))

    def snapshot(self):
        return fleet.SessionSnapshot(
            self.profile.node_id,
            self.profile.gdb_port,
            "attached",
            "stopped",
            100,
            "breakpoint-hit",
            "",
            "",
        )

    def close(self):
        self.closed = True


class FleetUnitTests(unittest.TestCase):
    def setUp(self):
        self.profiles = {
            "node-001": profile("node-001", 1234),
            "node-002": profile("node-002", 1235),
            "node-003": profile("node-003", 1236),
        }

    def test_profile_discovery_is_sorted_bounded_and_deduplicated(self):
        selected = fleet.discover_profiles([], self.profiles)
        self.assertEqual([item.node_id for item in selected], list(self.profiles))
        selected = fleet.discover_profiles(
            ["node-003", "node-001", "node-003"], self.profiles
        )
        self.assertEqual(
            [item.node_id for item in selected], ["node-003", "node-001"]
        )
        with self.assertRaisesRegex(fleet.FleetError, "unknown managed node"):
            fleet.discover_profiles(["node-999"], self.profiles)

    def test_duplicate_ports_are_rejected(self):
        duplicate = dict(self.profiles)
        duplicate["node-003"] = profile("node-003", 1234)
        with self.assertRaisesRegex(fleet.FleetError, "duplicate GDB ports"):
            fleet.discover_profiles([], duplicate)

    def test_session_bound_and_missing_tooling_fail_closed(self):
        too_many = {
            f"node-{index:03d}": profile(f"node-{index:03d}", 2000 + index)
            for index in range(1, fleet.MAX_FLEET_SESSIONS + 2)
        }
        with self.assertRaisesRegex(fleet.FleetError, "session count exceeds"):
            fleet.discover_profiles([], too_many)
        with tempfile.TemporaryDirectory() as directory:
            missing = Path(directory) / "vmlinux"
            with self.assertRaisesRegex(fleet.FleetError, "kernel symbols"):
                fleet.validate_tooling("gdb", missing)
            missing.write_bytes(b"symbols")
            with self.assertRaisesRegex(fleet.FleetError, "GDB executable"):
                fleet.validate_tooling("definitely-not-a-gdb-binary", missing)

    def test_selector_parsing(self):
        names = tuple(self.profiles)
        self.assertEqual(fleet.parse_selector("all", names), names)
        self.assertEqual(
            fleet.parse_selector("node-003,node-001,node-003", names),
            ("node-003", "node-001"),
        )
        with self.assertRaisesRegex(fleet.FleetError, "unknown selected node"):
            fleet.parse_selector("node-999", names)

    def test_mi_record_parsing_and_state_fields(self):
        self.assertIsNone(fleet.parse_mi_record("(gdb) \n"))
        stopped = fleet.parse_mi_record(
            '*stopped,reason="signal-received",signal-name="SIGSEGV"\n'
        )
        self.assertIsNotNone(stopped)
        assert stopped is not None
        self.assertEqual(stopped.prefix, "*")
        self.assertEqual(stopped.record_class, "stopped")
        self.assertEqual(stopped.results["reason"], "signal-received")
        self.assertEqual(stopped.results["signal-name"], "SIGSEGV")
        stream = fleet.parse_mi_record('~"#0 main ()\\n"\n')
        self.assertEqual(stream.text, "#0 main ()\n")
        malformed = fleet.parse_mi_record("not-mi\n")
        self.assertEqual(malformed.record_class, "malformed")

    def test_translation_uses_mi_and_bounded_console_pass_through(self):
        self.assertEqual(fleet.translate_gdb_command("continue"), "-exec-continue")
        self.assertEqual(fleet.translate_gdb_command("interrupt"), "-exec-interrupt")
        self.assertEqual(
            fleet.translate_gdb_command("info registers"),
            '-interpreter-exec console "info registers"',
        )

    def test_broadcast_single_and_multi_node_dispatch(self):
        output: list[str] = []
        controller = fleet.FleetController(
            tuple(self.profiles.values()),
            "/mock/gdb",
            Path("/mock/vmlinux"),
            session_factory=FakeSession,
            output=output.append,
        )
        controller.dispatch("continue")
        self.assertTrue(
            all(session.commands == ["continue"] for session in controller.sessions.values())
        )
        controller.handle_line("@node-001,node-003 bt")
        self.assertEqual(controller.sessions["node-001"].commands[-1], "bt")
        self.assertEqual(controller.sessions["node-003"].commands[-1], "bt")
        self.assertEqual(controller.sessions["node-002"].commands, ["continue"])
        controller.handle_line("select node-002")
        controller.handle_line("info registers")
        self.assertEqual(controller.sessions["node-002"].commands[-1], "info registers")

    def test_one_session_failure_does_not_block_others(self):
        FakeSession.failing_node = "node-002"
        try:
            controller = fleet.FleetController(
                tuple(self.profiles.values()),
                "/mock/gdb",
                Path("/mock/vmlinux"),
                session_factory=FakeSession,
                output=lambda _value: None,
            )
            results = controller.dispatch("bt")
            self.assertTrue(results["node-001"].ok)
            self.assertFalse(results["node-002"].ok)
            self.assertTrue(results["node-003"].ok)
        finally:
            FakeSession.failing_node = ""

    def test_status_is_compact_and_source_attributed(self):
        controller = fleet.FleetController(
            tuple(self.profiles.values()),
            "/mock/gdb",
            Path("/mock/vmlinux"),
            session_factory=FakeSession,
            output=lambda _value: None,
        )
        lines = controller.status_lines()
        self.assertIn("node-001\tattached\tstopped\t127.0.0.1:1234", lines)
        self.assertIn(
            "attached: 3/3  running: 0  stopped: 3  disconnected: 0  error: 0",
            lines,
        )

    def test_target_exit_becomes_explicitly_disconnected(self):
        session = fleet.GdbSession(
            self.profiles["node-001"], "/mock/gdb", Path("/mock/vmlinux")
        )
        session.gdb_state = "attached"
        session.target_state = "running"
        record = fleet.parse_mi_record('=thread-group-exited,id="i1"\n')
        assert record is not None
        session._handle_record(record)
        snapshot = session.snapshot()
        self.assertEqual(snapshot.gdb_state, "disconnected")
        self.assertEqual(snapshot.target_state, "exited")


class RealSubprocessProtocolTest(unittest.TestCase):
    def test_reader_thread_tracks_async_events_and_reaps_owned_process(self):
        events: list[fleet.SessionEvent] = []
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            symbols = root / "vmlinux"
            symbols.write_bytes(b"test-symbol-image")
            mock = root / "mock-gdb"
            mock.write_text(
                textwrap.dedent(
                    f"""\
                    #!{sys.executable}
                    import json
                    import re
                    import sys
                    for line in sys.stdin:
                        line = line.rstrip("\\n")
                        match = re.match(r"(\\d+)?(.*)", line)
                        token = match.group(1) or ""
                        command = match.group(2)
                        if command.startswith("-target-select"):
                            print('*stopped,reason="signal-received",signal-name="SIGTRAP"', flush=True)
                            print(token + "^connected", flush=True)
                        elif command == "-exec-continue":
                            print('*running,thread-id="all"', flush=True)
                            print(token + "^running", flush=True)
                        elif command == "-exec-interrupt":
                            print('*stopped,reason="signal-received",signal-name="SIGINT"', flush=True)
                            print(token + "^done", flush=True)
                        elif command.startswith("-interpreter-exec"):
                            print("~" + json.dumps("#0 mock_frame ()\\n"), flush=True)
                            print(token + "^done", flush=True)
                        elif command == "-gdb-exit":
                            print("^exit", flush=True)
                            break
                        else:
                            print(token + "^done", flush=True)
                    """
                ),
                encoding="utf-8",
            )
            mock.chmod(0o755)
            session = fleet.GdbSession(
                profile("node-001", 1234),
                str(mock),
                symbols,
                event_sink=events.append,
                command_timeout=2.0,
            )
            attached = session.attach(require_managed_vm=False)
            self.assertTrue(attached.ok, attached.error)
            self.assertEqual(session.snapshot().target_state, "stopped")
            continued = session.execute("continue")
            self.assertTrue(continued.ok, continued.error)
            self.assertTrue(session.wait_for_target("running", 1.0))
            interrupted = session.execute("interrupt")
            self.assertTrue(interrupted.ok, interrupted.error)
            self.assertTrue(session.wait_for_target("stopped", 1.0))
            result = session.execute("bt")
            self.assertTrue(result.ok)
            self.assertIn("#0 mock_frame ()\n", result.output)
            self.assertTrue(
                any(
                    item.kind == "stopped" and item.signal_name == "SIGINT"
                    for item in events
                )
            )
            process = session.process
            session.close()
            self.assertIsNotNone(process)
            assert process is not None
            self.assertIsNotNone(process.poll())
            self.assertEqual(session.snapshot().gdb_state, "disconnected")


if __name__ == "__main__":
    unittest.main()
