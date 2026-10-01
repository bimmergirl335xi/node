#!/usr/bin/env python3
"""Interactive bounded GDB/MI controller for managed Node QEMU guests."""

from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path
from typing import Callable, Mapping, Sequence

import node_lab


REPO_ROOT = Path(__file__).resolve().parent.parent
VMLINUX_PATH = REPO_ROOT / "build" / "artifacts" / "vmlinux"
GDB_LOOPBACK_HOST = "127.0.0.1"
MAX_FLEET_SESSIONS = 50
MAX_PARALLEL_OPERATIONS = 16
MAX_MI_LINE_BYTES = 64 * 1024
MAX_COMMAND_OUTPUT_BYTES = 256 * 1024
MAX_COMMAND_OUTPUT_RECORDS = 256
DEFAULT_COMMAND_TIMEOUT = 8.0
STATE_VALUES = {
    "starting", "connecting", "stopped", "running", "disconnected",
    "exited", "error", "unknown",
}
RESULT_PATTERN = re.compile(
    r'(?:^|,)([A-Za-z0-9_-]+)="((?:\\.|[^"\\])*)"'
)


class FleetError(RuntimeError):
    """An expected, user-facing fleet-controller failure."""


@dataclasses.dataclass(frozen=True)
class MiRecord:
    token: int | None
    prefix: str
    record_class: str
    results: Mapping[str, str]
    text: str


@dataclasses.dataclass(frozen=True)
class SessionEvent:
    node_id: str
    kind: str
    reason: str = ""
    signal_name: str = ""
    detail: str = ""


@dataclasses.dataclass(frozen=True)
class CommandResult:
    ok: bool
    result_class: str
    output: tuple[str, ...] = ()
    error: str = ""


@dataclasses.dataclass(frozen=True)
class SessionSnapshot:
    node_id: str
    port: int
    gdb_state: str
    target_state: str
    gdb_pid: int | None
    last_stop_reason: str
    last_signal: str
    last_error: str


@dataclasses.dataclass
class _PendingCommand:
    event: threading.Event = dataclasses.field(default_factory=threading.Event)
    result_class: str = ""
    results: dict[str, str] = dataclasses.field(default_factory=dict)
    output: list[str] = dataclasses.field(default_factory=list)
    output_bytes: int = 0
    truncated: bool = False

    def append(self, value: str) -> None:
        encoded_bytes = len(value.encode("utf-8", errors="replace"))
        if (
            len(self.output) >= MAX_COMMAND_OUTPUT_RECORDS
            or self.output_bytes + encoded_bytes > MAX_COMMAND_OUTPUT_BYTES
        ):
            self.truncated = True
            return
        self.output.append(value)
        self.output_bytes += encoded_bytes


def _decode_mi_string(value: str) -> str:
    if not value.startswith('"'):
        return value
    try:
        decoded = json.loads(value)
    except (json.JSONDecodeError, UnicodeError):
        return value[1:-1] if value.endswith('"') else value[1:]
    return decoded if isinstance(decoded, str) else str(decoded)


def parse_mi_record(line: str) -> MiRecord | None:
    """Parse the bounded MI record subset needed for state and diagnostics."""
    line = line.rstrip("\r\n")
    if not line or line.strip() == "(gdb)":
        return None
    encoded = line.encode("utf-8", errors="replace")
    if len(encoded) > MAX_MI_LINE_BYTES:
        return MiRecord(None, "!", "malformed", {}, "MI record exceeds bound")
    index = 0
    while index < len(line) and line[index].isdigit():
        index += 1
    token = int(line[:index]) if index else None
    if index >= len(line) or line[index] not in "^*+=~@&":
        return MiRecord(token, "!", "malformed", {}, line[:512])
    prefix = line[index]
    payload = line[index + 1 :]
    if prefix in "~@&":
        return MiRecord(token, prefix, "stream", {}, _decode_mi_string(payload))
    record_class = payload.split(",", 1)[0]
    results = {
        match.group(1): _decode_mi_string(f'"{match.group(2)}"')
        for match in RESULT_PATTERN.finditer(payload)
    }
    return MiRecord(token, prefix, record_class, results, line)


def discover_profiles(
    requested: Sequence[str],
    profiles: Mapping[str, node_lab.NodeProfile] | None = None,
) -> tuple[node_lab.NodeProfile, ...]:
    available = dict(node_lab.load_profiles() if profiles is None else profiles)
    names: list[str] = []
    if requested:
        for name in requested:
            if name not in available:
                raise FleetError(f"unknown managed node: {name}")
            if name not in names:
                names.append(name)
    else:
        names = sorted(available)
    if len(names) > MAX_FLEET_SESSIONS:
        raise FleetError(
            f"selected debugger session count exceeds {MAX_FLEET_SESSIONS}"
        )
    selected = tuple(available[name] for name in names)
    ports = [profile.gdb_port for profile in selected]
    if len(ports) != len(set(ports)):
        raise FleetError("selected managed nodes contain duplicate GDB ports")
    return selected


def parse_selector(
    value: str, available: Sequence[str]
) -> tuple[str, ...]:
    names = tuple(available)
    if value == "all":
        return names
    requested = value.split(",")
    if not requested or any(not item for item in requested):
        raise FleetError("selector must be all or a comma-separated node list")
    unknown = [item for item in requested if item not in names]
    if unknown:
        raise FleetError(f"unknown selected node: {unknown[0]}")
    return tuple(dict.fromkeys(requested))


def validate_tooling(gdb_value: str, vmlinux: Path = VMLINUX_PATH) -> str:
    if vmlinux.is_symlink() or not vmlinux.is_file() or vmlinux.stat().st_size <= 0:
        raise FleetError(
            f"kernel symbols are missing or empty; run ./scripts/build-node.sh: {vmlinux}"
        )
    resolved = shutil.which(gdb_value)
    if resolved is None:
        raise FleetError(f"GDB executable is unavailable: {gdb_value}")
    return resolved


def translate_gdb_command(command: str) -> str:
    stripped = command.strip()
    translations = {
        "continue": "-exec-continue",
        "c": "-exec-continue",
        "interrupt": "-exec-interrupt",
        "stepi": "-exec-step-instruction",
        "si": "-exec-step-instruction",
        "nexti": "-exec-next-instruction",
        "ni": "-exec-next-instruction",
    }
    if stripped in translations:
        return translations[stripped]
    return f"-interpreter-exec console {json.dumps(stripped)}"


class GdbSession:
    """One owned GDB/MI subprocess and its bounded observable state."""

    def __init__(
        self,
        profile: node_lab.NodeProfile,
        gdb_path: str,
        vmlinux: Path = VMLINUX_PATH,
        event_sink: Callable[[SessionEvent], None] | None = None,
        command_timeout: float = DEFAULT_COMMAND_TIMEOUT,
    ) -> None:
        self.profile = profile
        self.gdb_path = gdb_path
        self.vmlinux = vmlinux
        self.event_sink = event_sink or (lambda _event: None)
        self.command_timeout = command_timeout
        self.process: subprocess.Popen[str] | None = None
        self.reader: threading.Thread | None = None
        self._state_lock = threading.Lock()
        self._write_lock = threading.Lock()
        self._command_lock = threading.Lock()
        self._pending: dict[int, _PendingCommand] = {}
        self._active_token: int | None = None
        self._next_token = 1
        self._closing = False
        self.gdb_state = "disconnected"
        self.target_state = "unknown"
        self.last_stop_reason = ""
        self.last_signal = ""
        self.last_error = ""

    @property
    def node_id(self) -> str:
        return self.profile.node_id

    def snapshot(self) -> SessionSnapshot:
        with self._state_lock:
            process = self.process
            return SessionSnapshot(
                node_id=self.node_id,
                port=self.profile.gdb_port,
                gdb_state=self.gdb_state,
                target_state=self.target_state,
                gdb_pid=(process.pid if process is not None and process.poll() is None else None),
                last_stop_reason=self.last_stop_reason,
                last_signal=self.last_signal,
                last_error=self.last_error,
            )

    def _set_error(self, detail: str, *, target_state: str = "error") -> None:
        bounded = detail[:512]
        with self._state_lock:
            self.gdb_state = "error"
            self.target_state = target_state
            self.last_error = bounded
        self.event_sink(SessionEvent(self.node_id, "error", detail=bounded))

    def _verify_managed_debug_vm(self) -> str | None:
        paths = node_lab.instance_paths(self.profile)
        with node_lab.InstanceLock(paths):
            state, process, reason = node_lab.reconcile(self.profile, paths)
        if process is None:
            return f"managed VM is not running ({reason})"
        if not state or not state.get("debug_enabled"):
            return "managed VM is not running in debug mode"
        endpoint = state.get("gdb_endpoint")
        expected = f"{GDB_LOOPBACK_HOST}:{self.profile.gdb_port}"
        if endpoint != expected:
            return "managed VM debugger endpoint does not match its profile"
        return None

    def attach(self, *, require_managed_vm: bool = True) -> CommandResult:
        if self.process is not None and self.process.poll() is None:
            return CommandResult(False, "error", error="GDB session already exists")
        if require_managed_vm:
            problem = self._verify_managed_debug_vm()
            if problem:
                self._set_error(problem, target_state="unknown")
                return CommandResult(False, "error", error=problem)
        self._closing = False
        with self._state_lock:
            self.gdb_state = "starting"
            self.target_state = "unknown"
            self.last_error = ""
            self.last_signal = ""
            self.last_stop_reason = ""
        try:
            self.process = subprocess.Popen(
                [
                    self.gdb_path,
                    "--nx",
                    "--quiet",
                    "--interpreter=mi3",
                    str(self.vmlinux),
                ],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
                env={**os.environ, "LC_ALL": "C"},
            )
        except OSError as error:
            detail = f"cannot start GDB: {error}"
            self._set_error(detail, target_state="unknown")
            return CommandResult(False, "error", error=detail)
        self.reader = threading.Thread(
            target=self._reader_loop,
            name=f"gdb-mi-{self.node_id}",
            daemon=True,
        )
        self.reader.start()
        with self._state_lock:
            self.gdb_state = "connecting"
        for command in (
            "-gdb-set pagination off",
            "-gdb-set confirm off",
            "-gdb-set mi-async on",
        ):
            result = self._mi_command(command)
            if not result.ok:
                self.close()
                self._set_error(result.error or "GDB initialization failed")
                return result
        result = self._mi_command(
            f"-target-select remote {GDB_LOOPBACK_HOST}:{self.profile.gdb_port}"
        )
        if not result.ok:
            error = result.error or "GDB target connection failed"
            self.close()
            self._set_error(error, target_state="disconnected")
            return CommandResult(False, result.result_class, result.output, error)
        with self._state_lock:
            self.gdb_state = "attached"
            if self.target_state == "unknown":
                self.target_state = "stopped"
                self.last_stop_reason = "initial-debug-stop"
        self.event_sink(SessionEvent(self.node_id, "attached", reason="stopped"))
        return CommandResult(True, result.result_class, result.output)

    def _reader_loop(self) -> None:
        process = self.process
        if process is None or process.stdout is None:
            return
        try:
            for line in process.stdout:
                record = parse_mi_record(line)
                if record is not None:
                    self._handle_record(record)
        except (OSError, UnicodeError) as error:
            if not self._closing:
                self._set_error(f"GDB output failure: {error}", target_state="unknown")
        finally:
            self._fail_pending("GDB exited before completing the command")
            if not self._closing:
                return_code = process.poll()
                with self._state_lock:
                    self.gdb_state = "exited"
                    self.target_state = "unknown"
                    self.last_error = f"GDB exited unexpectedly ({return_code})"
                self.event_sink(
                    SessionEvent(
                        self.node_id,
                        "error",
                        detail=f"GDB exited unexpectedly ({return_code})",
                    )
                )

    def _fail_pending(self, detail: str) -> None:
        with self._state_lock:
            pending = tuple(self._pending.values())
            for item in pending:
                item.result_class = "error"
                item.results["msg"] = detail
                item.event.set()

    def _handle_record(self, record: MiRecord) -> None:
        if record.prefix in "~@&":
            with self._state_lock:
                pending = self._pending.get(self._active_token or -1)
                if pending is not None:
                    pending.append(record.text)
            return
        if record.prefix == "^" and record.token is not None:
            with self._state_lock:
                pending = self._pending.get(record.token)
                if pending is not None:
                    pending.result_class = record.record_class
                    pending.results.update(record.results)
                    pending.event.set()
            return
        if record.prefix == "*" and record.record_class == "running":
            with self._state_lock:
                self.target_state = "running"
                self.last_stop_reason = ""
                self.last_signal = ""
            self.event_sink(SessionEvent(self.node_id, "running"))
            return
        if record.prefix == "*" and record.record_class == "stopped":
            reason = record.results.get("reason", "unknown")[:128]
            signal_name = record.results.get("signal-name", "")[:64]
            target_state = "exited" if reason.startswith("exited") else "stopped"
            with self._state_lock:
                self.target_state = target_state
                if target_state == "exited":
                    self.gdb_state = "disconnected"
                self.last_stop_reason = reason
                self.last_signal = signal_name
            self.event_sink(
                SessionEvent(self.node_id, target_state, reason, signal_name)
            )
            return
        if record.prefix == "=" and record.record_class == "thread-group-exited":
            with self._state_lock:
                self.gdb_state = "disconnected"
                self.target_state = "exited"
                self.last_stop_reason = "thread-group-exited"
            self.event_sink(SessionEvent(self.node_id, "exited", "thread-group-exited"))
            return
        if record.prefix == "=" and record.record_class in (
            "thread-created", "thread-exited"
        ):
            self.event_sink(
                SessionEvent(self.node_id, record.record_class, detail=record.text[:512])
            )
            return
        if record.prefix == "!":
            self.event_sink(SessionEvent(self.node_id, "malformed", detail=record.text))

    def _mi_command(self, command: str) -> CommandResult:
        process = self.process
        if process is None or process.poll() is not None or process.stdin is None:
            return CommandResult(False, "error", error="GDB session is not running")
        with self._command_lock:
            with self._state_lock:
                token = self._next_token
                self._next_token += 1
                pending = _PendingCommand()
                self._pending[token] = pending
                self._active_token = token
            try:
                with self._write_lock:
                    process.stdin.write(f"{token}{command}\n")
                    process.stdin.flush()
            except (BrokenPipeError, OSError) as error:
                with self._state_lock:
                    self._pending.pop(token, None)
                    self._active_token = None
                detail = f"cannot write GDB command: {error}"
                self._set_error(detail, target_state="unknown")
                return CommandResult(False, "error", error=detail)
            if not pending.event.wait(self.command_timeout):
                with self._state_lock:
                    self._pending.pop(token, None)
                    self._active_token = None
                detail = f"GDB command timed out after {self.command_timeout:g}s"
                self._set_error(detail, target_state="unknown")
                return CommandResult(False, "error", tuple(pending.output), detail)
            with self._state_lock:
                self._pending.pop(token, None)
                self._active_token = None
            output = list(pending.output)
            if pending.truncated:
                output.append("[bounded GDB output truncated]")
            error = pending.results.get("msg", "")[:512]
            ok = pending.result_class not in ("error", "exit", "")
            return CommandResult(ok, pending.result_class, tuple(output), error)

    def execute(self, command: str) -> CommandResult:
        snapshot = self.snapshot()
        if snapshot.gdb_state != "attached":
            return CommandResult(
                False,
                "error",
                error=f"debugger is {snapshot.gdb_state}",
            )
        return self._mi_command(translate_gdb_command(command))

    def wait_for_target(self, state: str, timeout: float = 5.0) -> bool:
        if state not in STATE_VALUES:
            raise ValueError(f"unsupported target state: {state}")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.snapshot().target_state == state:
                return True
            time.sleep(0.02)
        return self.snapshot().target_state == state

    def close(self) -> None:
        self._closing = True
        process = self.process
        if process is None:
            with self._state_lock:
                self.gdb_state = "disconnected"
                self.target_state = "unknown"
            return
        if process.poll() is None and process.stdin is not None:
            try:
                with self._write_lock:
                    process.stdin.write("-gdb-exit\n")
                    process.stdin.flush()
            except (BrokenPipeError, OSError):
                pass
        try:
            process.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                process.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait(timeout=1.0)
        if self.reader is not None and self.reader is not threading.current_thread():
            self.reader.join(timeout=1.0)
        for stream in (process.stdin, process.stdout):
            if stream is not None:
                try:
                    stream.close()
                except OSError:
                    pass
        self._fail_pending("GDB session closed")
        with self._state_lock:
            self.gdb_state = "disconnected"
            self.target_state = "unknown"


class FleetController:
    def __init__(
        self,
        profiles: Sequence[node_lab.NodeProfile],
        gdb_path: str,
        vmlinux: Path = VMLINUX_PATH,
        session_factory: Callable[..., GdbSession] = GdbSession,
        output: Callable[[str], None] = print,
    ) -> None:
        self.profiles = tuple(profiles)
        self.node_names = tuple(profile.node_id for profile in self.profiles)
        self.scope = self.node_names
        self.output = output
        self._output_lock = threading.Lock()
        self._interactive = False
        self.sessions = {
            profile.node_id: session_factory(
                profile, gdb_path, vmlinux, event_sink=self._asynchronous_event
            )
            for profile in self.profiles
        }

    def prompt(self) -> str:
        label = "all" if self.scope == self.node_names else ",".join(self.scope)
        return f"[gdb:{label}]> "

    def _emit(self, value: str) -> None:
        with self._output_lock:
            self.output(value)

    def _asynchronous_event(self, event: SessionEvent) -> None:
        parts = [f"[{event.node_id}] {event.kind}"]
        if event.reason:
            parts.append(f"reason={event.reason}")
        if event.signal_name:
            parts.append(f"signal={event.signal_name}")
        if event.detail:
            parts.append(event.detail)
        with self._output_lock:
            self.output(" ".join(parts))
            if self._interactive and sys.stdout.isatty():
                # GNU readline is not thread-safe. A plain redraw keeps the
                # prompt visible without calling it from MI reader threads.
                sys.stdout.write(self.prompt())
                sys.stdout.flush()

    def connect_all(self) -> dict[str, CommandResult]:
        return self._parallel(
            self.node_names,
            lambda session: session.attach(),
        )

    def _parallel(
        self,
        names: Sequence[str],
        operation: Callable[[GdbSession], CommandResult],
    ) -> dict[str, CommandResult]:
        results: dict[str, CommandResult] = {}
        if not names:
            return results
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=min(len(names), MAX_PARALLEL_OPERATIONS)
        ) as executor:
            futures = {
                executor.submit(operation, self.sessions[name]): name
                for name in names
            }
            for future in concurrent.futures.as_completed(futures):
                name = futures[future]
                try:
                    results[name] = future.result()
                except Exception as error:  # contain one session failure
                    results[name] = CommandResult(
                        False, "error", error=f"contained session failure: {error}"
                    )
        return {name: results[name] for name in names}

    def dispatch(
        self, command: str, names: Sequence[str] | None = None
    ) -> dict[str, CommandResult]:
        targets = tuple(self.scope if names is None else names)
        results = self._parallel(targets, lambda session: session.execute(command))
        for name, result in results.items():
            if result.output:
                self._emit(f"[{name}]")
                for line in result.output:
                    for part in line.rstrip("\n").splitlines() or [""]:
                        self._emit(part)
            self._emit(
                f"[{name}] {'sent' if result.ok else 'failed'}"
                + (f": {result.error}" if result.error else "")
            )
        return results

    def select(self, selector: str) -> tuple[str, ...]:
        self.scope = parse_selector(selector, self.node_names)
        return self.scope

    def status_lines(self) -> tuple[str, ...]:
        snapshots = [self.sessions[name].snapshot() for name in self.node_names]
        lines = ["NODE\tGDB\tTARGET\tENDPOINT"]
        for item in snapshots:
            lines.append(
                f"{item.node_id}\t{item.gdb_state}\t{item.target_state}\t"
                f"{GDB_LOOPBACK_HOST}:{item.port}"
            )
            if item.last_error:
                lines.append(f"  error: {item.last_error}")
        attached = sum(item.gdb_state == "attached" for item in snapshots)
        running = sum(item.target_state == "running" for item in snapshots)
        stopped = sum(item.target_state == "stopped" for item in snapshots)
        errors = sum(item.gdb_state in ("error", "exited") for item in snapshots)
        disconnected = sum(item.gdb_state == "disconnected" for item in snapshots)
        lines.append(
            f"attached: {attached}/{len(snapshots)}  running: {running}  "
            f"stopped: {stopped}  disconnected: {disconnected}  error: {errors}"
        )
        return tuple(lines)

    def print_status(self) -> None:
        for line in self.status_lines():
            self._emit(line)

    def reconnect(self, names: Sequence[str]) -> dict[str, CommandResult]:
        def operation(session: GdbSession) -> CommandResult:
            session.close()
            return session.attach()
        return self._parallel(names, operation)

    def disconnect(self, names: Sequence[str]) -> None:
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=min(max(len(names), 1), MAX_PARALLEL_OPERATIONS)
        ) as executor:
            tuple(executor.map(lambda name: self.sessions[name].close(), names))

    def close(self) -> None:
        self.disconnect(self.node_names)

    def _controller_help(self) -> str:
        return (
            "fleet commands: status | nodes | select all|NODE[,NODE] | "
            "reconnect [all|NODE[,NODE]] | disconnect [all|NODE[,NODE]] | "
            "help | quit\n"
            "targeted GDB command: @NODE[,NODE] COMMAND\n"
            "other input is passed to GDB in the current selection"
        )

    def handle_line(self, line: str) -> bool:
        stripped = line.strip()
        if not stripped:
            return True
        if stripped in ("quit", "exit"):
            return False
        if stripped == "help":
            self._emit(self._controller_help())
            return True
        if stripped in ("status", "nodes"):
            self.print_status()
            return True
        if stripped.startswith("select "):
            selected = self.select(stripped.split(None, 1)[1])
            self._emit(f"selected: {','.join(selected)}")
            return True
        for controller_command in ("reconnect", "disconnect"):
            if stripped == controller_command or stripped.startswith(
                controller_command + " "
            ):
                selector = (
                    stripped.split(None, 1)[1]
                    if " " in stripped
                    else ",".join(self.scope)
                )
                names = parse_selector(selector, self.node_names)
                if controller_command == "disconnect":
                    self.disconnect(names)
                    for name in names:
                        self._emit(f"[{name}] disconnected")
                else:
                    results = self.reconnect(names)
                    for name, result in results.items():
                        self._emit(
                            f"[{name}] {'attached' if result.ok else 'failed'}"
                            + (f": {result.error}" if result.error else "")
                        )
                return True
        targets: Sequence[str] | None = None
        command = stripped
        if stripped.startswith("@"):
            try:
                selector, command = stripped[1:].split(None, 1)
            except ValueError as error:
                raise FleetError("targeted command requires @SELECTOR COMMAND") from error
            targets = parse_selector(selector, self.node_names)
        self.dispatch(command, targets)
        return True

    def run(self) -> None:
        self._interactive = True
        try:
            self.print_status()
            while True:
                try:
                    line = input(self.prompt())
                except EOFError:
                    break
                except KeyboardInterrupt:
                    self._emit("interrupt requested for current selection")
                    self.dispatch("interrupt")
                    continue
                try:
                    if not self.handle_line(line):
                        break
                except FleetError as error:
                    self._emit(f"fleet error: {error}")
        finally:
            self._interactive = False


def argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Control managed Node QEMU GDB sessions through GDB/MI3."
    )
    parser.add_argument("nodes", nargs="*", help="optional managed-node subset")
    parser.add_argument("--gdb", default="gdb", help="GDB executable (default: gdb)")
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = argument_parser().parse_args(argv)
    controller: FleetController | None = None
    try:
        profiles = discover_profiles(args.nodes)
        gdb_path = validate_tooling(args.gdb)
        controller = FleetController(profiles, gdb_path)
        results = controller.connect_all()
        for name, result in results.items():
            if not result.ok:
                print(f"[{name}] attach failed: {result.error}", file=sys.stderr)
        controller.run()
        return 0
    except (FleetError, node_lab.LabError, OSError) as error:
        print(f"Node GDB fleet error: {error}", file=sys.stderr)
        return 1
    finally:
        if controller is not None:
            controller.close()


if __name__ == "__main__":
    raise SystemExit(main())
