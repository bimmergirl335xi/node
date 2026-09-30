#!/usr/bin/env python3
"""Bounded local controller for disposable Node QEMU development instances."""

from __future__ import annotations

import argparse
import dataclasses
import datetime
import fcntl
import hashlib
import itertools
import json
import os
import re
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Iterator


REPO_ROOT = Path(__file__).resolve().parent.parent
PROFILE_ROOT = REPO_ROOT / "config" / "virtual_nodes"
BUILD_ROOT = REPO_ROOT / "build"
INSTANCE_ROOT = BUILD_ROOT / "virtual"
ISO_PATH = BUILD_ROOT / "artifacts" / "node-current.iso"
VMLINUX_PATH = BUILD_ROOT / "artifacts" / "vmlinux"
RUN_LAUNCHER = REPO_ROOT / "scripts" / "run-node-qemu.sh"
DEBUG_LAUNCHER = REPO_ROOT / "scripts" / "debug-node-qemu.sh"

PROFILE_KEYS = {
    "node_id",
    "firmware",
    "vcpus",
    "memory_mb",
    "debug_enabled",
    "gdb_port",
    "network_enabled",
    "network_peer",
    "mac_address",
}
NODE_ID_PATTERN = re.compile(r"[a-z0-9][a-z0-9-]{0,31}\Z")
MAC_PATTERN = re.compile(r"02:([0-9a-f]{2}:){4}[0-9a-f]{2}\Z")
MAX_PROFILES = 64
MAX_PROFILE_BYTES = 16 * 1024
MAX_STATE_BYTES = 32 * 1024
MAX_EVENT_BYTES = 512 * 1024
MAX_EVENT_LINES = 256
MAX_EVENT_LINE_BYTES = 8192
MAX_SERIAL_TAIL_BYTES = 256 * 1024
MAX_SERIAL_LINES = 200
MAX_PROCESS_INSPECTIONS = 131072
START_WAIT_SECONDS = 8.0
STOP_WAIT_SECONDS = 5.0
ACS_IPV4 = {
    "node-001": "10.77.0.1/24",
    "node-002": "10.77.0.2/24",
}
ACS_UDP_PORT = 39001


class LabError(RuntimeError):
    """An expected, user-facing controller failure."""


@dataclasses.dataclass(frozen=True)
class NodeProfile:
    node_id: str
    firmware: str
    vcpus: int
    memory_mb: int
    debug_enabled: bool
    gdb_port: int
    network_enabled: bool
    network_peer: str
    mac_address: str


@dataclasses.dataclass(frozen=True)
class InstancePaths:
    root: Path
    state_dir: Path
    log_dir: Path
    state_file: Path
    pid_file: Path
    lock_file: Path
    serial_log: Path
    event_log: Path
    controller_log: Path
    network_socket: Path


@dataclasses.dataclass(frozen=True)
class ProcessInfo:
    pid: int
    state: str
    ppid: int
    start_ticks: int
    argv: tuple[str, ...]


def utc_now() -> str:
    return datetime.datetime.now(datetime.timezone.utc).replace(
        microsecond=0
    ).isoformat().replace("+00:00", "Z")


def bounded_json(path: Path, limit: int, description: str) -> Any:
    try:
        if path.is_symlink() or not path.is_file():
            raise LabError(f"{description} is not a regular file: {path}")
        with path.open("rb") as source:
            content = source.read(limit + 1)
        if not content or len(content) > limit:
            raise LabError(f"{description} size is outside its bounded limit: {path}")
        return json.loads(content.decode("utf-8"))
    except UnicodeError as error:
        raise LabError(f"{description} is not valid UTF-8: {path}") from error
    except json.JSONDecodeError as error:
        raise LabError(f"{description} is malformed JSON: {path}: {error}") from error
    except OSError as error:
        raise LabError(f"cannot read {description}: {path}: {error}") from error


def require_int(value: Any, name: str, minimum: int, maximum: int) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise LabError(f"profile field {name} must be an integer")
    if value < minimum or value > maximum:
        raise LabError(f"profile field {name} must be from {minimum} through {maximum}")
    return value


def load_profiles() -> dict[str, NodeProfile]:
    try:
        files = sorted(
            itertools.islice(PROFILE_ROOT.glob("*.json"), MAX_PROFILES + 1)
        )
    except OSError as error:
        raise LabError(f"cannot enumerate virtual-node profiles: {error}") from error
    if not files:
        raise LabError(f"no virtual-node profiles found under {PROFILE_ROOT}")
    if len(files) > MAX_PROFILES:
        raise LabError(f"virtual-node profile count exceeds {MAX_PROFILES}")

    profiles: dict[str, NodeProfile] = {}
    for path in files:
        raw = bounded_json(path, MAX_PROFILE_BYTES, "virtual-node profile")
        if not isinstance(raw, dict) or set(raw) != PROFILE_KEYS:
            raise LabError(f"profile must contain exactly the supported fields: {path}")
        node_id = raw["node_id"]
        firmware = raw["firmware"]
        debug_enabled = raw["debug_enabled"]
        network_enabled = raw["network_enabled"]
        network_peer = raw["network_peer"]
        mac_address = raw["mac_address"]
        if not isinstance(node_id, str) or NODE_ID_PATTERN.fullmatch(node_id) is None:
            raise LabError(f"profile node_id is invalid: {path}")
        if path.stem != node_id:
            raise LabError(f"profile filename must match node_id: {path}")
        if firmware not in ("bios", "uefi"):
            raise LabError(f"profile firmware must be bios or uefi: {path}")
        if not isinstance(debug_enabled, bool):
            raise LabError(f"profile debug_enabled must be a boolean: {path}")
        if not isinstance(network_enabled, bool) or not network_enabled:
            raise LabError(f"profile network_enabled must be true: {path}")
        if (
            not isinstance(network_peer, str)
            or NODE_ID_PATTERN.fullmatch(network_peer) is None
            or network_peer == node_id
        ):
            raise LabError(f"profile network_peer is invalid: {path}")
        if not isinstance(mac_address, str) or MAC_PATTERN.fullmatch(mac_address) is None:
            raise LabError(f"profile mac_address is invalid: {path}")
        profile = NodeProfile(
            node_id=node_id,
            firmware=firmware,
            vcpus=require_int(raw["vcpus"], "vcpus", 1, 16),
            memory_mb=require_int(raw["memory_mb"], "memory_mb", 128, 8192),
            debug_enabled=debug_enabled,
            gdb_port=require_int(raw["gdb_port"], "gdb_port", 1024, 65535),
            network_enabled=network_enabled,
            network_peer=network_peer,
            mac_address=mac_address,
        )
        if node_id in profiles:
            raise LabError(f"duplicate virtual-node identity: {node_id}")
        profiles[node_id] = profile
    gdb_ports = [profile.gdb_port for profile in profiles.values()]
    if len(gdb_ports) != len(set(gdb_ports)):
        raise LabError("virtual-node GDB ports must be unique")
    mac_addresses = [profile.mac_address for profile in profiles.values()]
    if len(mac_addresses) != len(set(mac_addresses)):
        raise LabError("virtual-node MAC addresses must be unique")
    for profile in profiles.values():
        peer = profiles.get(profile.network_peer)
        if peer is None or peer.network_peer != profile.node_id:
            raise LabError(
                f"virtual-node network peer must be present and reciprocal: {profile.node_id}"
            )
    return profiles


def select_profile(node_id: str) -> NodeProfile:
    profiles = load_profiles()
    try:
        return profiles[node_id]
    except KeyError as error:
        raise LabError(f"unknown managed virtual node: {node_id}") from error


def instance_paths(profile: NodeProfile) -> InstancePaths:
    root = INSTANCE_ROOT / profile.node_id
    state_dir = root / "state"
    log_dir = root / "logs"
    return InstancePaths(
        root=root,
        state_dir=state_dir,
        log_dir=log_dir,
        state_file=state_dir / "node.json",
        pid_file=state_dir / "qemu.pid",
        lock_file=state_dir / "controller.lock",
        serial_log=log_dir / "qemu-last-run.log",
        event_log=log_dir / "qemu-boot-events.jsonl",
        controller_log=log_dir / "controller-launch.log",
        network_socket=state_dir / "lab-network.sock",
    )


def ensure_instance_dirs(paths: InstancePaths) -> None:
    paths.state_dir.mkdir(parents=True, exist_ok=True, mode=0o755)
    paths.log_dir.mkdir(parents=True, exist_ok=True, mode=0o755)


def locked(paths: InstancePaths) -> Iterator[None]:
    ensure_instance_dirs(paths)
    lock = paths.lock_file.open("a+", encoding="utf-8")
    try:
        fcntl.flock(lock.fileno(), fcntl.LOCK_EX)
        yield
    finally:
        fcntl.flock(lock.fileno(), fcntl.LOCK_UN)
        lock.close()


class InstanceLock:
    def __init__(self, paths: InstancePaths):
        self._iterator = locked(paths)

    def __enter__(self) -> None:
        next(self._iterator)

    def __exit__(self, exc_type: Any, exc: Any, traceback: Any) -> None:
        try:
            next(self._iterator)
        except StopIteration:
            pass


def load_state(paths: InstancePaths, profile: NodeProfile) -> dict[str, Any] | None:
    if not paths.state_file.exists():
        return None
    raw = bounded_json(paths.state_file, MAX_STATE_BYTES, "managed-node state")
    if not isinstance(raw, dict) or raw.get("node_id") != profile.node_id:
        raise LabError(f"managed-node state identity is invalid: {paths.state_file}")
    return raw


def atomic_text(path: Path, text: str, mode: int = 0o644) -> None:
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o755)
    temporary_name = ""
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", dir=path.parent, prefix=f".{path.name}.",
            delete=False
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(text)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.chmod(temporary_name, mode)
        os.replace(temporary_name, path)
    finally:
        if temporary_name:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass


def write_state(paths: InstancePaths, state: dict[str, Any]) -> None:
    encoded = json.dumps(state, sort_keys=True, indent=2) + "\n"
    if len(encoded.encode("utf-8")) > MAX_STATE_BYTES:
        raise LabError("managed-node state exceeds its bounded limit")
    atomic_text(paths.state_file, encoded)


def remove_pid_file(paths: InstancePaths) -> None:
    try:
        paths.pid_file.unlink()
    except FileNotFoundError:
        pass


def process_info(pid: int) -> ProcessInfo | None:
    if pid <= 0:
        return None
    proc = Path("/proc") / str(pid)
    try:
        stat_text = (proc / "stat").read_text(encoding="ascii")
        close = stat_text.rfind(")")
        if close < 0:
            return None
        fields = stat_text[close + 2 :].split()
        if len(fields) < 20:
            return None
        argv_bytes = (proc / "cmdline").read_bytes()
        argv = tuple(
            item.decode("utf-8", errors="surrogateescape")
            for item in argv_bytes.rstrip(b"\0").split(b"\0")
            if item
        )
        return ProcessInfo(
            pid=pid,
            state=fields[0],
            ppid=int(fields[1]),
            start_ticks=int(fields[19]),
            argv=argv,
        )
    except (FileNotFoundError, ProcessLookupError, PermissionError, ValueError, OSError):
        return None


def has_argument_pair(argv: tuple[str, ...], option: str, value: str) -> bool:
    return any(
        argv[index] == option and argv[index + 1] == value
        for index in range(len(argv) - 1)
    )


def network_arguments(profile: NodeProfile) -> tuple[str, str]:
    local = instance_paths(profile).network_socket
    peer = INSTANCE_ROOT / profile.network_peer / "state" / "lab-network.sock"
    netdev = (
        "dgram,id=node_lab_net,"
        f"local.type=unix,local.path={local},"
        f"remote.type=unix,remote.path={peer}"
    )
    device = f"virtio-net-pci,netdev=node_lab_net,mac={profile.mac_address}"
    return netdev, device


def network_attachment(profile: NodeProfile, running: ProcessInfo | None) -> str:
    if running is None:
        return "configured"
    try:
        return "attached" if instance_paths(profile).network_socket.is_socket() else "missing"
    except OSError:
        return "unknown"


def matches_managed_qemu(
    info: ProcessInfo, profile: NodeProfile, expected_iso: Path
) -> bool:
    if info.state == "Z" or not info.argv:
        return False
    executable = Path(info.argv[0]).name
    marker = f"guest={profile.node_id},process={profile.node_id}"
    netdev, device = network_arguments(profile)
    smbios = f"type=1,product=Node-Development-VM,serial={profile.node_id}"
    return (
        executable.startswith("qemu-system-")
        and has_argument_pair(info.argv, "-name", marker)
        and has_argument_pair(info.argv, "-cdrom", str(expected_iso))
        and has_argument_pair(info.argv, "-netdev", netdev)
        and has_argument_pair(info.argv, "-device", device)
        and has_argument_pair(info.argv, "-smbios", smbios)
    )


def is_descendant(pid: int, ancestor_pid: int) -> bool:
    current = pid
    visited: set[int] = set()
    for _ in range(64):
        if current == ancestor_pid:
            return True
        if current <= 1 or current in visited:
            return False
        visited.add(current)
        info = process_info(current)
        if info is None:
            return False
        current = info.ppid
    return False


def find_managed_qemu(
    profile: NodeProfile, expected_iso: Path, ancestor_pid: int | None = None
) -> list[ProcessInfo]:
    matches: list[ProcessInfo] = []
    try:
        entries = Path("/proc").iterdir()
    except OSError as error:
        raise LabError(f"cannot inspect host process table: {error}") from error
    inspected = 0
    for entry in entries:
        if not entry.name.isdecimal():
            continue
        inspected += 1
        if inspected > MAX_PROCESS_INSPECTIONS:
            raise LabError("host process table exceeds the bounded inspection limit")
        info = process_info(int(entry.name))
        if info is None or not matches_managed_qemu(info, profile, expected_iso):
            continue
        if ancestor_pid is None or is_descendant(info.pid, ancestor_pid):
            matches.append(info)
    return sorted(matches, key=lambda item: item.pid)


def state_process(
    state: dict[str, Any] | None, profile: NodeProfile
) -> tuple[ProcessInfo | None, str]:
    if state is None:
        return None, "no_state"
    pid = state.get("qemu_pid")
    start_ticks = state.get("qemu_start_ticks")
    if isinstance(pid, bool) or not isinstance(pid, int) or pid <= 0:
        return None, "no_recorded_pid"
    if isinstance(start_ticks, bool) or not isinstance(start_ticks, int):
        return None, "invalid_start_identity"
    info = process_info(pid)
    if info is None or info.state == "Z":
        return None, "process_exited"
    if info.start_ticks != start_ticks:
        return None, "pid_reused"
    if not matches_managed_qemu(info, profile, ISO_PATH):
        return None, "process_identity_mismatch"
    return info, "running"


def launcher_process(state: dict[str, Any] | None) -> ProcessInfo | None:
    if state is None:
        return None
    pid = state.get("launcher_pid")
    start_ticks = state.get("launcher_start_ticks")
    if (
        isinstance(pid, bool)
        or not isinstance(pid, int)
        or isinstance(start_ticks, bool)
        or not isinstance(start_ticks, int)
    ):
        return None
    info = process_info(pid)
    if info is None or info.state == "Z" or info.start_ticks != start_ticks:
        return None
    return info


def wait_for_launcher_exit(state: dict[str, Any] | None) -> bool:
    deadline = time.monotonic() + 2.0
    while time.monotonic() < deadline:
        if launcher_process(state) is None:
            return True
        time.sleep(0.05)
    return launcher_process(state) is None


def stopped_state(
    profile: NodeProfile, previous: dict[str, Any] | None, reason: str
) -> dict[str, Any]:
    state = dict(previous or {})
    state.update(
        {
            "schema": "node-managed-instance-state-v1",
            "node_id": profile.node_id,
            "status": "stopped",
            "qemu_pid": None,
            "qemu_start_ticks": None,
            "last_observed_utc": utc_now(),
            "stop_reason": reason,
        }
    )
    return state


def reconcile(
    profile: NodeProfile, paths: InstancePaths
) -> tuple[dict[str, Any] | None, ProcessInfo | None, str]:
    state = load_state(paths, profile)
    info, reason = state_process(state, profile)
    if state is not None and info is None and state.get("status") != "stopped":
        state = stopped_state(profile, state, reason)
        write_state(paths, state)
        remove_pid_file(paths)
    return state, info, reason


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    try:
        with path.open("rb") as source:
            for block in iter(lambda: source.read(1024 * 1024), b""):
                digest.update(block)
    except OSError as error:
        raise LabError(f"cannot hash canonical ISO: {error}") from error
    return digest.hexdigest()


def validate_iso() -> str:
    try:
        valid = (
            not ISO_PATH.is_symlink()
            and ISO_PATH.is_file()
            and ISO_PATH.stat().st_size > 0
        )
    except OSError as error:
        raise LabError(f"cannot inspect canonical ISO: {error}") from error
    if not valid:
        raise LabError(
            f"canonical ISO is missing or empty; run ./scripts/build-node.sh first: {ISO_PATH}"
        )
    return sha256_file(ISO_PATH)


def launch_command(profile: NodeProfile, debug: bool) -> list[str]:
    firmware = f"--{profile.firmware}"
    if debug:
        return [str(DEBUG_LAUNCHER), firmware, "--port", str(profile.gdb_port)]
    return [str(RUN_LAUNCHER), firmware]


def start_node(profile: NodeProfile, force_debug: bool) -> None:
    paths = instance_paths(profile)
    with InstanceLock(paths):
        iso_sha = validate_iso()
        state, running, _ = reconcile(profile, paths)
        if running is not None:
            raise LabError(f"{profile.node_id} is already running as QEMU PID {running.pid}")
        if not wait_for_launcher_exit(state):
            raise LabError(
                f"previous launcher for {profile.node_id} is still finalizing logs"
            )
        unrecorded = find_managed_qemu(profile, ISO_PATH)
        if unrecorded:
            pids = ", ".join(str(item.pid) for item in unrecorded)
            raise LabError(
                f"refusing to start while unrecorded matching QEMU process exists: {pids}"
            )

        debug = force_debug or profile.debug_enabled
        if debug and (VMLINUX_PATH.is_symlink() or not VMLINUX_PATH.is_file()):
            raise LabError(
                f"kernel symbols are missing; run ./scripts/build-node.sh first: {VMLINUX_PATH}"
            )
        ensure_instance_dirs(paths)
        remove_pid_file(paths)
        environment = os.environ.copy()
        for name in (
            "NODE_QEMU_BIN",
            "NODE_QEMU_DEBUG_MODE",
            "NODE_QEMU_GDB_PORT",
            "NODE_QEMU_LOG_DIR",
            "NODE_QEMU_NODE_ID",
            "NODE_QEMU_VCPUS",
            "NODE_QEMU_MEMORY_MB",
            "NODE_QEMU_TIMEOUT_SECONDS",
            "NODE_QEMU_NETWORK_ENABLED",
            "NODE_QEMU_NETWORK_MAC",
            "NODE_QEMU_NETWORK_LOCAL_SOCKET",
            "NODE_QEMU_NETWORK_PEER_SOCKET",
        ):
            environment.pop(name, None)
        environment.update(
            {
                "NODE_QEMU_LOG_DIR": str(paths.log_dir),
                "NODE_QEMU_NODE_ID": profile.node_id,
                "NODE_QEMU_VCPUS": str(profile.vcpus),
                "NODE_QEMU_MEMORY_MB": str(profile.memory_mb),
                "NODE_QEMU_TIMEOUT_SECONDS": "600" if debug else "90",
                "NODE_QEMU_NETWORK_ENABLED": "1",
                "NODE_QEMU_NETWORK_MAC": profile.mac_address,
                "NODE_QEMU_NETWORK_LOCAL_SOCKET": str(paths.network_socket),
                "NODE_QEMU_NETWORK_PEER_SOCKET": str(
                    INSTANCE_ROOT
                    / profile.network_peer
                    / "state"
                    / "lab-network.sock"
                ),
            }
        )
        command = launch_command(profile, debug)
        try:
            controller_log = paths.controller_log.open("wb")
            try:
                launcher = subprocess.Popen(
                    command,
                    cwd=REPO_ROOT,
                    env=environment,
                    stdin=subprocess.DEVNULL,
                    stdout=controller_log,
                    stderr=subprocess.STDOUT,
                    start_new_session=True,
                    close_fds=True,
                )
            finally:
                controller_log.close()
        except OSError as error:
            raise LabError(f"cannot start QEMU launcher: {error}") from error

        launcher_info = process_info(launcher.pid)
        provisional = {
            "schema": "node-managed-instance-state-v1",
            "node_id": profile.node_id,
            "status": "starting",
            "launcher_pid": launcher.pid,
            "launcher_start_ticks": (
                launcher_info.start_ticks if launcher_info is not None else None
            ),
            "qemu_pid": None,
            "qemu_start_ticks": None,
            "firmware": profile.firmware,
            "vcpus": profile.vcpus,
            "memory_mb": profile.memory_mb,
            "network_backend": "unix_dgram_point_to_point",
            "network_peer": profile.network_peer,
            "network_socket": str(paths.network_socket),
            "mac_address": profile.mac_address,
            "debug_enabled": debug,
            "gdb_endpoint": f"127.0.0.1:{profile.gdb_port}" if debug else None,
            "iso_path": str(ISO_PATH),
            "iso_sha256": iso_sha,
            "started_utc": utc_now(),
            "last_observed_utc": utc_now(),
        }
        write_state(paths, provisional)

        deadline = time.monotonic() + START_WAIT_SECONDS
        qemu: ProcessInfo | None = None
        while time.monotonic() < deadline:
            matches = find_managed_qemu(profile, ISO_PATH, launcher.pid)
            if len(matches) > 1:
                os.killpg(launcher.pid, signal.SIGTERM)
                write_state(paths, stopped_state(profile, provisional, "ambiguous_processes"))
                raise LabError("multiple QEMU child processes matched the managed identity")
            if matches:
                qemu = matches[0]
                break
            if launcher.poll() is not None:
                break
            time.sleep(0.05)

        if qemu is None:
            if launcher.poll() is None:
                try:
                    os.killpg(launcher.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            write_state(paths, stopped_state(profile, provisional, "launcher_failed"))
            raise LabError(
                f"QEMU did not establish a managed process; inspect {paths.controller_log}"
            )

        # Do not report a transient exec as a running instance. Immediate
        # launcher failures (for example, a rejected debug socket) can expose
        # a QEMU PID briefly before the process reports its error.
        time.sleep(0.2)
        confirmed = process_info(qemu.pid)
        if (
            confirmed is None
            or confirmed.start_ticks != qemu.start_ticks
            or not matches_managed_qemu(confirmed, profile, ISO_PATH)
        ):
            write_state(paths, stopped_state(profile, provisional, "qemu_start_failed"))
            raise LabError(
                f"QEMU exited during startup; inspect {paths.controller_log}"
            )
        qemu = confirmed

        running_state = dict(provisional)
        running_state.update(
            {
                "status": "running",
                "qemu_pid": qemu.pid,
                "qemu_start_ticks": qemu.start_ticks,
                "last_observed_utc": utc_now(),
            }
        )
        write_state(paths, running_state)
        atomic_text(paths.pid_file, f"{qemu.pid}\n")
        print(f"started: {profile.node_id}")
        print(f"qemu_pid: {qemu.pid}")
        print(f"firmware: {profile.firmware}")
        print(f"debug_enabled: {str(debug).lower()}")
        print(f"network_attachment: {network_attachment(profile, qemu)}")
        print(f"network_peer: {profile.network_peer}")
        print(f"mac_address: {profile.mac_address}")
        print(f"acs_reference_address: {ACS_IPV4[profile.node_id]}")
        print(f"acs_reference_udp_port: {ACS_UDP_PORT}")
        print(f"serial_log: {paths.serial_log}")
        print(f"event_log: {paths.event_log}")


def stop_node(profile: NodeProfile, allow_stopped: bool = True) -> None:
    paths = instance_paths(profile)
    with InstanceLock(paths):
        state = load_state(paths, profile)
        info, reason = state_process(state, profile)
        if info is None:
            if state is not None:
                write_state(paths, stopped_state(profile, state, reason))
            remove_pid_file(paths)
            if reason in ("pid_reused", "process_identity_mismatch", "invalid_start_identity"):
                raise LabError(
                    f"refused to signal recorded PID because identity verification failed: {reason}"
                )
            if allow_stopped:
                print(f"already stopped: {profile.node_id}")
                return
            raise LabError(f"{profile.node_id} is already stopped ({reason})")

        try:
            os.kill(info.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        deadline = time.monotonic() + STOP_WAIT_SECONDS
        while time.monotonic() < deadline:
            current = process_info(info.pid)
            if (
                current is None
                or current.state == "Z"
                or current.start_ticks != info.start_ticks
            ):
                break
            time.sleep(0.05)
        else:
            current = process_info(info.pid)
            if (
                current is not None
                and current.start_ticks == info.start_ticks
                and matches_managed_qemu(current, profile, ISO_PATH)
            ):
                os.kill(info.pid, signal.SIGKILL)
                kill_deadline = time.monotonic() + 1.0
                while time.monotonic() < kill_deadline:
                    killed = process_info(info.pid)
                    if (
                        killed is None
                        or killed.state == "Z"
                        or killed.start_ticks != info.start_ticks
                    ):
                        break
                    time.sleep(0.05)
                else:
                    raise LabError(
                        f"QEMU PID {info.pid} did not stop after bounded termination"
                    )

        launcher_finished = wait_for_launcher_exit(state)
        write_state(
            paths,
            stopped_state(
                profile,
                state,
                "operator_stop" if launcher_finished else "launcher_finalizing",
            ),
        )
        remove_pid_file(paths)
        print(f"stopped: {profile.node_id}")
        print(f"qemu_pid: {info.pid}")
        if not launcher_finished:
            print("launcher_state: finalizing")


def read_events(paths: InstancePaths) -> tuple[list[dict[str, Any]], list[str]]:
    if not paths.event_log.exists():
        return [], []
    try:
        if paths.event_log.is_symlink() or not paths.event_log.is_file():
            raise LabError(f"event log is not a regular file: {paths.event_log}")
        with paths.event_log.open("rb") as source:
            data = source.read(MAX_EVENT_BYTES + 1)
    except OSError as error:
        raise LabError(f"cannot read event log: {error}") from error
    if len(data) > MAX_EVENT_BYTES:
        raise LabError(f"event log exceeds the {MAX_EVENT_BYTES}-byte bound")
    lines = data.splitlines()
    if len(lines) > MAX_EVENT_LINES:
        raise LabError(f"event log exceeds the {MAX_EVENT_LINES}-record bound")
    events: list[dict[str, Any]] = []
    malformed: list[str] = []
    for number, line in enumerate(lines, start=1):
        if len(line) > MAX_EVENT_LINE_BYTES:
            malformed.append(f"line {number}: exceeds {MAX_EVENT_LINE_BYTES} bytes")
            continue
        try:
            item = json.loads(line.decode("utf-8"))
        except (UnicodeError, json.JSONDecodeError) as error:
            malformed.append(f"line {number}: malformed JSON ({error})")
            continue
        if not isinstance(item, dict):
            malformed.append(f"line {number}: event must be a JSON object")
            continue
        events.append(item)
    return events, malformed


def event_summary(paths: InstancePaths) -> tuple[int, str, str]:
    try:
        events, malformed = read_events(paths)
    except LabError:
        return 0, "unavailable", "unavailable"
    final = "not_observed"
    shutdown = "not_observed"
    for event in events:
        if event.get("record") == "micro_os_final_result":
            final = str(event.get("outcome", "unknown"))[:128]
        if event.get("record") == "shutdown_result":
            shutdown = str(event.get("outcome", "unknown"))[:128]
    if malformed:
        final = f"{final}; malformed={len(malformed)}"
    return len(events), final, shutdown


def print_status(profile: NodeProfile) -> None:
    paths = instance_paths(profile)
    with InstanceLock(paths):
        state, info, reason = reconcile(profile, paths)
        event_count, final, shutdown = event_summary(paths)
        iso_exists = ISO_PATH.is_file() and not ISO_PATH.is_symlink()
        iso_sha = sha256_file(ISO_PATH) if iso_exists else "unavailable"
        debug = bool(info is not None and state and state.get("debug_enabled"))
        endpoint = (
            str(state.get("gdb_endpoint"))
            if debug and state and state.get("gdb_endpoint")
            else "disabled"
        )
        detail = (
            reason
            if info is not None or not state
            else str(state.get("stop_reason", reason))[:128]
        )
        print(f"node_id: {profile.node_id}")
        print(f"running: {str(info is not None).lower()}")
        print(f"state: {'running' if info is not None else 'stopped'}")
        print(f"state_detail: {detail}")
        print(f"qemu_pid: {info.pid if info is not None else '-'}")
        print(f"firmware: {profile.firmware}")
        print(f"vcpus: {profile.vcpus}")
        print(f"memory_mb: {profile.memory_mb}")
        print(f"iso_path: {ISO_PATH}")
        print(f"iso_sha256: {iso_sha}")
        print(f"event_count: {event_count}")
        print(f"latest_final_outcome: {final}")
        print(f"latest_shutdown_outcome: {shutdown}")
        print(f"boot_state: {final}")
        print(f"network_attachment: {network_attachment(profile, info)}")
        print(f"network_backend: unix_dgram_point_to_point")
        print(f"network_peer: {profile.network_peer}")
        print(f"mac_address: {profile.mac_address}")
        print(f"acs_reference_address: {ACS_IPV4[profile.node_id]}")
        print(f"acs_reference_udp_port: {ACS_UDP_PORT}")
        print(f"serial_log: {paths.serial_log}")
        print(f"event_log: {paths.event_log}")
        print(f"debug_enabled: {str(debug).lower()}")
        print(f"gdb_endpoint: {endpoint}")


def print_list() -> None:
    profiles = load_profiles()
    print("NODE ID\tSTATE\tPID\tFIRMWARE\tLAST BOOT RESULT\tNETWORK")
    for node_id in sorted(profiles):
        profile = profiles[node_id]
        paths = instance_paths(profile)
        with InstanceLock(paths):
            _, info, _ = reconcile(profile, paths)
            _, final, _ = event_summary(paths)
        print(
            f"{node_id}\t{'running' if info else 'stopped'}\t"
            f"{info.pid if info else '-'}\t{profile.firmware}\t{final}\t"
            f"{network_attachment(profile, info)}"
        )


def print_events(profile: NodeProfile, limit: int) -> None:
    paths = instance_paths(profile)
    events, malformed = read_events(paths)
    if not paths.event_log.exists():
        raise LabError(f"event log is not available: {paths.event_log}")
    for event in events[-limit:]:
        print(json.dumps(event, separators=(",", ":"), sort_keys=True))
    for message in malformed:
        print(f"MALFORMED {message}", file=sys.stderr)
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")


def print_acs_events(profile: NodeProfile, limit: int) -> None:
    paths = instance_paths(profile)
    events, malformed = read_events(paths)
    if not paths.event_log.exists():
        raise LabError(f"event log is not available: {paths.event_log}")
    selected = [
        event for event in events
        if isinstance(event.get("record"), str)
        and event["record"].startswith("acs_")
    ]
    if not selected:
        raise LabError(f"ACS evidence is not available: {paths.event_log}")
    for event in selected[-limit:]:
        print(json.dumps(event, separators=(",", ":"), sort_keys=True))
    for message in malformed:
        print(f"MALFORMED {message}", file=sys.stderr)
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")


def print_serial(profile: NodeProfile, lines: int) -> None:
    path = instance_paths(profile).serial_log
    try:
        if path.is_symlink() or not path.is_file():
            raise LabError(f"serial log is not available: {path}")
        size = path.stat().st_size
        with path.open("rb") as source:
            if size > MAX_SERIAL_TAIL_BYTES:
                source.seek(-MAX_SERIAL_TAIL_BYTES, os.SEEK_END)
                source.readline()
            data = source.read(MAX_SERIAL_TAIL_BYTES + 1)
    except OSError as error:
        raise LabError(f"cannot read serial log: {error}") from error
    if len(data) > MAX_SERIAL_TAIL_BYTES:
        data = data[-MAX_SERIAL_TAIL_BYTES:]
    selected = data.decode("utf-8", errors="replace").splitlines()[-lines:]
    for line in selected:
        print(line)


def print_debug_info(profile: NodeProfile) -> None:
    paths = instance_paths(profile)
    with InstanceLock(paths):
        state, info, _ = reconcile(profile, paths)
    active_debug = bool(state.get("debug_enabled")) if state else False
    print(f"node_id: {profile.node_id}")
    print(f"debug_supported: true")
    print(f"profile_debug_enabled: {str(profile.debug_enabled).lower()}")
    print(f"active_debug_session: {str(active_debug and info is not None).lower()}")
    print(f"gdb_endpoint: 127.0.0.1:{profile.gdb_port}")
    print(f"symbols: {VMLINUX_PATH}")
    print(f"start_command: ./python/node_lab.py start {profile.node_id} --debug")
    print(f"gdb_command: gdb {VMLINUX_PATH}")
    print(f"gdb_attach: target remote 127.0.0.1:{profile.gdb_port}")


def bounded_count(value: str, maximum: int) -> int:
    try:
        count = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("must be an integer") from error
    if count < 1 or count > maximum:
        raise argparse.ArgumentTypeError(f"must be from 1 through {maximum}")
    return count


def parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(
        description="Manage bounded disposable Node QEMU development instances."
    )
    commands = root.add_subparsers(dest="command", required=True)
    commands.add_parser("list", help="list configured managed virtual nodes")
    for name in ("status", "stop", "debug-info"):
        command = commands.add_parser(name)
        command.add_argument("node_id")
    for name in ("start", "restart"):
        command = commands.add_parser(name)
        command.add_argument("node_id")
        command.add_argument("--debug", action="store_true")
    events = commands.add_parser("events")
    events.add_argument("node_id")
    events.add_argument(
        "--limit", type=lambda value: bounded_count(value, MAX_EVENT_LINES), default=20
    )
    acs_events = commands.add_parser("acs-events")
    acs_events.add_argument("node_id")
    acs_events.add_argument(
        "--limit", type=lambda value: bounded_count(value, MAX_EVENT_LINES), default=20
    )
    serial = commands.add_parser("serial")
    serial.add_argument("node_id")
    serial.add_argument(
        "--lines", type=lambda value: bounded_count(value, MAX_SERIAL_LINES), default=80
    )
    return root


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        if args.command == "list":
            print_list()
            return 0
        profile = select_profile(args.node_id)
        if args.command == "status":
            print_status(profile)
        elif args.command == "start":
            start_node(profile, args.debug)
        elif args.command == "stop":
            stop_node(profile)
        elif args.command == "restart":
            stop_node(profile, allow_stopped=True)
            start_node(profile, args.debug)
        elif args.command == "events":
            print_events(profile, args.limit)
        elif args.command == "acs-events":
            print_acs_events(profile, args.limit)
        elif args.command == "serial":
            print_serial(profile, args.lines)
        elif args.command == "debug-info":
            print_debug_info(profile)
        return 0
    except LabError as error:
        print(f"node-lab error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
