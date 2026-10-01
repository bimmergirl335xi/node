#!/usr/bin/env python3
"""Bounded local controller for disposable Node QEMU development instances."""

from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import datetime
import fcntl
import hashlib
import ipaddress
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
    "cpu_sockets",
    "cpu_cores",
    "cpu_threads",
    "memory_mb",
    "debug_enabled",
    "gdb_port",
    "network_enabled",
    "mac_address",
    "ipv4_address",
    "acs_port",
    "network_multicast_address",
    "network_multicast_port",
    "network_local_address",
    "acs_mode",
    "acs_peers",
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
MAX_PEERS = 16
MAX_VCPUS = 16


class LabError(RuntimeError):
    """An expected, user-facing controller failure."""


@dataclasses.dataclass(frozen=True)
class NodeProfile:
    node_id: str
    firmware: str
    vcpus: int
    cpu_sockets: int
    cpu_cores: int
    cpu_threads: int
    memory_mb: int
    debug_enabled: bool
    gdb_port: int
    network_enabled: bool
    mac_address: str
    ipv4_address: str
    acs_port: int
    network_multicast_address: str
    network_multicast_port: int
    network_local_address: str
    acs_mode: str
    acs_peers: tuple[str, ...]


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


@dataclasses.dataclass(frozen=True)
class ProcessInfo:
    pid: int
    state: str
    ppid: int
    start_ticks: int
    argv: tuple[str, ...]


@dataclasses.dataclass(frozen=True)
class ExplicitPeerSummary:
    valid: frozenset[str]
    expected: int
    result: str


@dataclasses.dataclass(frozen=True)
class DiscoveryObservationSummary:
    direct: int | None
    hint: int | None
    stale: int | None
    conflict: int | None
    result: str


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


def validate_cpu_topology(
    vcpus: Any, sockets: Any, cores: Any, threads: Any
) -> tuple[int, int, int, int]:
    total = require_int(vcpus, "vcpus", 1, MAX_VCPUS)
    socket_count = require_int(sockets, "cpu_sockets", 1, MAX_VCPUS)
    core_count = require_int(cores, "cpu_cores", 1, MAX_VCPUS)
    thread_count = require_int(threads, "cpu_threads", 1, MAX_VCPUS)
    if socket_count > MAX_VCPUS // core_count:
        raise LabError("profile CPU topology product exceeds the bounded vCPU limit")
    socket_cores = socket_count * core_count
    if socket_cores > MAX_VCPUS // thread_count:
        raise LabError("profile CPU topology product exceeds the bounded vCPU limit")
    if socket_cores * thread_count != total:
        raise LabError("profile vcpus must equal cpu_sockets * cpu_cores * cpu_threads")
    return total, socket_count, core_count, thread_count


def cpu_discovery_counts_consistent(
    expected: int, configured: int, online: int, allowed: int
) -> bool:
    return expected == configured and 1 <= allowed <= online <= configured


def load_profiles(profile_root: Path = PROFILE_ROOT) -> dict[str, NodeProfile]:
    try:
        files = sorted(
            itertools.islice(profile_root.glob("*.json"), MAX_PROFILES + 1)
        )
    except OSError as error:
        raise LabError(f"cannot enumerate virtual-node profiles: {error}") from error
    if not files:
        raise LabError(f"no virtual-node profiles found under {profile_root}")
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
        mac_address = raw["mac_address"]
        ipv4_address = raw["ipv4_address"]
        multicast_address = raw["network_multicast_address"]
        local_address = raw["network_local_address"]
        acs_mode = raw["acs_mode"]
        acs_peers = raw["acs_peers"]
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
        if not isinstance(mac_address, str) or MAC_PATTERN.fullmatch(mac_address) is None:
            raise LabError(f"profile mac_address is invalid: {path}")
        try:
            parsed_ipv4 = ipaddress.IPv4Interface(ipv4_address)
            parsed_multicast = ipaddress.IPv4Address(multicast_address)
            parsed_local = ipaddress.IPv4Address(local_address)
        except (TypeError, ValueError) as error:
            raise LabError(f"profile network address is invalid: {path}: {error}") from error
        if parsed_ipv4.network.prefixlen != 24 or parsed_ipv4.ip.is_unspecified:
            raise LabError(f"profile ipv4_address must be a usable /24 address: {path}")
        if not parsed_multicast.is_multicast:
            raise LabError(f"profile network_multicast_address must be multicast: {path}")
        if not parsed_local.is_loopback:
            raise LabError(f"profile network_local_address must be loopback: {path}")
        if acs_mode not in ("explicit", "discovery"):
            raise LabError(f"profile acs_mode must be explicit or discovery: {path}")
        if (
            not isinstance(acs_peers, list)
            or len(acs_peers) > MAX_PEERS
            or any(
                not isinstance(peer, str)
                or NODE_ID_PATTERN.fullmatch(peer) is None
                or peer == node_id
                for peer in acs_peers
            )
            or acs_peers != sorted(set(acs_peers))
        ):
            raise LabError(f"profile acs_peers must be a bounded sorted unique list: {path}")
        vcpus, cpu_sockets, cpu_cores, cpu_threads = validate_cpu_topology(
            raw["vcpus"], raw["cpu_sockets"], raw["cpu_cores"], raw["cpu_threads"]
        )
        profile = NodeProfile(
            node_id=node_id,
            firmware=firmware,
            vcpus=vcpus,
            cpu_sockets=cpu_sockets,
            cpu_cores=cpu_cores,
            cpu_threads=cpu_threads,
            memory_mb=require_int(raw["memory_mb"], "memory_mb", 128, 8192),
            debug_enabled=debug_enabled,
            gdb_port=require_int(raw["gdb_port"], "gdb_port", 1024, 65535),
            network_enabled=network_enabled,
            mac_address=mac_address,
            ipv4_address=str(parsed_ipv4),
            acs_port=require_int(raw["acs_port"], "acs_port", 1024, 65535),
            network_multicast_address=str(parsed_multicast),
            network_multicast_port=require_int(
                raw["network_multicast_port"], "network_multicast_port", 1024, 65535
            ),
            network_local_address=str(parsed_local),
            acs_mode=acs_mode,
            acs_peers=tuple(acs_peers),
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
    ipv4_addresses = [profile.ipv4_address for profile in profiles.values()]
    if len(ipv4_addresses) != len(set(ipv4_addresses)):
        raise LabError("virtual-node IPv4 addresses must be unique")
    network_shapes = {
        (profile.network_multicast_address, profile.network_multicast_port,
         profile.network_local_address, profile.acs_port)
        for profile in profiles.values()
    }
    if len(network_shapes) != 1:
        raise LabError("virtual-node profiles must declare one shared network shape")
    expected = set(profiles)
    for profile in profiles.values():
        unknown_peers = set(profile.acs_peers) - expected
        if unknown_peers:
            raise LabError(
                f"virtual-node ACS peers name unknown profiles: {profile.node_id}"
            )
        if profile.acs_mode == "explicit" and (
            not profile.acs_peers or
            set(profile.acs_peers) != expected - {profile.node_id}
        ):
            raise LabError(
                f"explicit-mode ACS peers must name every other profile: {profile.node_id}"
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
    try:
        if not paths.state_file.is_symlink() and paths.state_file.is_file() and \
                paths.state_file.stat().st_size == 0:
            return None
    except OSError as error:
        raise LabError(f"cannot inspect managed-node state: {error}") from error
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
    netdev = (
        "dgram,id=node_lab_net,"
        f"remote.type=inet,remote.host={profile.network_multicast_address},"
        f"remote.port={profile.network_multicast_port},"
        f"local.type=inet,local.host={profile.network_local_address},"
        f"local.port={profile.network_multicast_port}"
    )
    device = f"virtio-net-pci,netdev=node_lab_net,mac={profile.mac_address}"
    return netdev, device


def network_attachment(profile: NodeProfile, running: ProcessInfo | None) -> str:
    return "attached" if running is not None else "configured"


def profile_peer_encoding(profile: NodeProfile, profiles: dict[str, NodeProfile]) -> str:
    return ";".join(
        f"{peer}@{profiles[peer].ipv4_address.split('/', 1)[0]}"
        for peer in profile.acs_peers
    )


def profile_peer_parts(
    profile: NodeProfile, profiles: dict[str, NodeProfile], acs_mode: str
) -> tuple[str, str]:
    if acs_mode == "discovery":
        return "", ""
    entries = profile_peer_encoding(profile, profiles).split(";")
    midpoint = (len(entries) + 1) // 2
    return ";".join(entries[:midpoint]), ";".join(entries[midpoint:])


def smbios_argument(
    profile: NodeProfile, profiles: dict[str, NodeProfile], mode: str,
    acs_mode: str,
) -> str:
    peers_a, peers_b = profile_peer_parts(profile, profiles, acs_mode)
    if acs_mode == "discovery":
        return (
            f"type=1,manufacturer=none,product=Node-Development-VM-{mode},"
            f"version=acs-discovery-v1-port-{profile.acs_port},"
            f"serial={profile.node_id},sku={profile.ipv4_address.split('/', 1)[0]},"
            "family=none"
        )
    return (
        f"type=1,manufacturer={peers_a},product=Node-Development-VM-{mode},"
        f"version=acs-profile-v1-port-{profile.acs_port},"
        f"serial={profile.node_id},sku={profile.ipv4_address.split('/', 1)[0]},"
        f"family={peers_b}"
    )


def matches_managed_qemu(
    info: ProcessInfo, profile: NodeProfile, expected_iso: Path
) -> bool:
    if info.state == "Z" or not info.argv:
        return False
    executable = Path(info.argv[0]).name
    marker = f"guest={profile.node_id},process={profile.node_id}"
    netdev, device = network_arguments(profile)
    profiles = load_profiles()
    smbios_values = tuple(
        smbios_argument(profile, profiles, mode, acs_mode)
        for mode in ("conformance", "lab")
        for acs_mode in ("explicit", "discovery")
    )
    return (
        executable.startswith("qemu-system-")
        and has_argument_pair(info.argv, "-name", marker)
        and has_argument_pair(info.argv, "-cdrom", str(expected_iso))
        and has_argument_pair(info.argv, "-netdev", netdev)
        and has_argument_pair(info.argv, "-device", device)
        and any(
            has_argument_pair(info.argv, "-smbios", value)
            for value in smbios_values
        )
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


def start_node(
    profile: NodeProfile, force_debug: bool, *, emit_output: bool = True,
    known_iso_sha: str | None = None, mode: str = "conformance",
    acs_mode: str | None = None,
) -> dict[str, Any]:
    if mode not in ("conformance", "lab"):
        raise LabError("node mode must be conformance or lab")
    selected_acs_mode = profile.acs_mode if acs_mode is None else acs_mode
    if selected_acs_mode not in ("explicit", "discovery"):
        raise LabError("ACS mode must be explicit or discovery")
    paths = instance_paths(profile)
    with InstanceLock(paths):
        iso_sha = known_iso_sha if known_iso_sha is not None else validate_iso()
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
            "NODE_QEMU_CPU_SOCKETS",
            "NODE_QEMU_CPU_CORES",
            "NODE_QEMU_CPU_THREADS",
            "NODE_QEMU_MEMORY_MB",
            "NODE_QEMU_TIMEOUT_SECONDS",
            "NODE_QEMU_MODE",
            "NODE_QEMU_NETWORK_ENABLED",
            "NODE_QEMU_NETWORK_MAC",
            "NODE_QEMU_NETWORK_MULTICAST_ADDRESS",
            "NODE_QEMU_NETWORK_MULTICAST_PORT",
            "NODE_QEMU_NETWORK_LOCAL_ADDRESS",
            "NODE_QEMU_ACS_IPV4_ADDRESS",
            "NODE_QEMU_ACS_UDP_PORT",
            "NODE_QEMU_ACS_MODE",
            "NODE_QEMU_ACS_PEERS_A",
            "NODE_QEMU_ACS_PEERS_B",
        ):
            environment.pop(name, None)
        peers_a, peers_b = profile_peer_parts(
            profile, load_profiles(), selected_acs_mode
        )
        environment.update(
            {
                "NODE_QEMU_LOG_DIR": str(paths.log_dir),
                "NODE_QEMU_NODE_ID": profile.node_id,
                "NODE_QEMU_VCPUS": str(profile.vcpus),
                "NODE_QEMU_CPU_SOCKETS": str(profile.cpu_sockets),
                "NODE_QEMU_CPU_CORES": str(profile.cpu_cores),
                "NODE_QEMU_CPU_THREADS": str(profile.cpu_threads),
                "NODE_QEMU_MEMORY_MB": str(profile.memory_mb),
                "NODE_QEMU_TIMEOUT_SECONDS": (
                    "0" if mode == "lab" else ("600" if debug else "90")
                ),
                "NODE_QEMU_MODE": mode,
                "NODE_QEMU_NETWORK_ENABLED": "1",
                "NODE_QEMU_NETWORK_MAC": profile.mac_address,
                "NODE_QEMU_NETWORK_MULTICAST_ADDRESS": profile.network_multicast_address,
                "NODE_QEMU_NETWORK_MULTICAST_PORT": str(profile.network_multicast_port),
                "NODE_QEMU_NETWORK_LOCAL_ADDRESS": profile.network_local_address,
                "NODE_QEMU_ACS_IPV4_ADDRESS": profile.ipv4_address.split("/", 1)[0],
                "NODE_QEMU_ACS_UDP_PORT": str(profile.acs_port),
                "NODE_QEMU_ACS_MODE": selected_acs_mode,
                "NODE_QEMU_ACS_PEERS_A": peers_a,
                "NODE_QEMU_ACS_PEERS_B": peers_b,
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
            "cpu_sockets": profile.cpu_sockets,
            "cpu_cores": profile.cpu_cores,
            "cpu_threads": profile.cpu_threads,
            "memory_mb": profile.memory_mb,
            "mode": mode,
            "acs_mode": selected_acs_mode,
            "network_backend": "loopback_multicast_dgram_lan",
            "network_multicast_address": profile.network_multicast_address,
            "network_multicast_port": profile.network_multicast_port,
            "acs_peers": (
                list(profile.acs_peers) if selected_acs_mode == "explicit" else []
            ),
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
        result = {
            "node_id": profile.node_id,
            "qemu_pid": qemu.pid,
            "firmware": profile.firmware,
            "mode": mode,
            "acs_mode": selected_acs_mode,
            "vcpus": profile.vcpus,
            "cpu_sockets": profile.cpu_sockets,
            "cpu_cores": profile.cpu_cores,
            "cpu_threads": profile.cpu_threads,
            "debug_enabled": debug,
            "network_attachment": network_attachment(profile, qemu),
            "acs_peers": (
                list(profile.acs_peers) if selected_acs_mode == "explicit" else []
            ),
            "mac_address": profile.mac_address,
            "acs_reference_address": profile.ipv4_address,
            "acs_reference_udp_port": profile.acs_port,
            "serial_log": str(paths.serial_log),
            "event_log": str(paths.event_log),
        }
        if emit_output:
            print_start_result(result)
        return result


def print_start_result(result: dict[str, Any]) -> None:
    print(f"started: {result['node_id']}")
    for key in (
        "qemu_pid", "firmware", "mode", "acs_mode", "vcpus", "cpu_sockets", "cpu_cores",
        "cpu_threads", "debug_enabled", "network_attachment",
        "acs_peers", "mac_address", "acs_reference_address",
        "acs_reference_udp_port", "serial_log", "event_log"
    ):
        value = result[key]
        if isinstance(value, bool):
            value = str(value).lower()
        elif isinstance(value, list):
            value = ",".join(value)
        print(f"{key}: {value}")


def stop_node(
    profile: NodeProfile, allow_stopped: bool = True, *, emit_output: bool = True
) -> dict[str, Any]:
    paths = instance_paths(profile)
    with InstanceLock(paths):
        state = load_state(paths, profile)
        info, reason = state_process(state, profile)
        if info is None:
            active_launcher = launcher_process(state)
            if active_launcher is not None:
                recovered = find_managed_qemu(
                    profile, ISO_PATH, ancestor_pid=active_launcher.pid
                )
                if len(recovered) > 1:
                    raise LabError(
                        "refused to stop multiple QEMU children under the recorded launcher"
                    )
                if recovered:
                    info = recovered[0]
                    reason = "recovered_launcher_child"
        if info is None:
            if state is not None:
                write_state(paths, stopped_state(profile, state, reason))
            remove_pid_file(paths)
            if reason in ("pid_reused", "process_identity_mismatch", "invalid_start_identity"):
                raise LabError(
                    f"refused to signal recorded PID because identity verification failed: {reason}"
                )
            if allow_stopped:
                result = {"node_id": profile.node_id, "already_stopped": True, "qemu_pid": None}
                if emit_output:
                    print(f"already stopped: {profile.node_id}")
                return result
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
        result = {
            "node_id": profile.node_id,
            "already_stopped": False,
            "qemu_pid": info.pid,
            "launcher_finalizing": not launcher_finished,
        }
        if emit_output:
            print(f"stopped: {profile.node_id}")
            print(f"qemu_pid: {info.pid}")
            if not launcher_finished:
                print("launcher_state: finalizing")
        return result


def read_events(paths: InstancePaths) -> tuple[list[dict[str, Any]], list[str]]:
    live_serial = not paths.event_log.exists() and paths.serial_log.exists()
    source_path = paths.serial_log if live_serial else paths.event_log
    if not source_path.exists():
        return [], []
    try:
        if source_path.is_symlink() or not source_path.is_file():
            raise LabError(f"event source is not a regular file: {source_path}")
        with source_path.open("rb") as source:
            data = source.read(MAX_EVENT_BYTES + 1)
    except OSError as error:
        raise LabError(f"cannot read event log: {error}") from error
    if len(data) > MAX_EVENT_BYTES:
        raise LabError(f"event log exceeds the {MAX_EVENT_BYTES}-byte bound")
    lines = data.splitlines()
    if live_serial:
        lines = [
            line for line in lines
            if line.startswith(b'{"record":"') and line.endswith(b"}")
        ]
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
        print(f"mode: {str((state or {}).get('mode', 'unknown'))[:32]}")
        selected_acs_mode = str((state or {}).get("acs_mode", profile.acs_mode))[:32]
        print(f"acs_mode: {selected_acs_mode}")
        print(f"firmware: {profile.firmware}")
        print(f"vcpus: {profile.vcpus}")
        print(f"cpu_sockets: {profile.cpu_sockets}")
        print(f"cpu_cores: {profile.cpu_cores}")
        print(f"cpu_threads: {profile.cpu_threads}")
        print(f"memory_mb: {profile.memory_mb}")
        print(f"iso_path: {ISO_PATH}")
        print(f"iso_sha256: {iso_sha}")
        print(f"event_count: {event_count}")
        print(f"latest_final_outcome: {final}")
        print(f"latest_shutdown_outcome: {shutdown}")
        print(f"boot_state: {final}")
        print(f"network_attachment: {network_attachment(profile, info)}")
        print("network_backend: loopback_multicast_dgram_lan")
        selected_peers = (
            profile.acs_peers if selected_acs_mode == "explicit" else ()
        )
        print(f"acs_peers: {','.join(selected_peers)}")
        print(f"mac_address: {profile.mac_address}")
        print(f"acs_reference_address: {profile.ipv4_address}")
        print(f"acs_reference_udp_port: {profile.acs_port}")
        print(f"serial_log: {paths.serial_log}")
        print(f"event_log: {paths.event_log}")
        print(f"debug_enabled: {str(debug).lower()}")
        print(f"gdb_endpoint: {endpoint}")


def print_list() -> None:
    profiles = load_profiles()
    print("NODE ID\tSTATE\tPID\tMODE\tACS MODE\tFIRMWARE\tLAST BOOT RESULT\tNETWORK")
    for node_id in sorted(profiles):
        profile = profiles[node_id]
        paths = instance_paths(profile)
        with InstanceLock(paths):
            state, info, _ = reconcile(profile, paths)
            _, final, _ = event_summary(paths)
        print(
            f"{node_id}\t{'running' if info else 'stopped'}\t"
            f"{info.pid if info else '-'}\t"
            f"{str((state or {}).get('mode', 'unknown'))[:32]}\t"
            f"{str((state or {}).get('acs_mode', profile.acs_mode))[:32]}\t"
            f"{profile.firmware}\t{final}\t"
            f"{network_attachment(profile, info)}"
        )


def run_group_operation(
    operation: str, force_debug: bool = False, mode: str = "conformance",
    acs_mode: str | None = None,
) -> None:
    profiles = load_profiles()
    ordered = [profiles[node_id] for node_id in sorted(profiles)]
    iso_sha = validate_iso() if operation in ("start", "restart") else None

    def invoke(profile: NodeProfile) -> dict[str, Any]:
        if operation == "start":
            return start_node(
                profile, force_debug, emit_output=False, known_iso_sha=iso_sha,
                mode=mode,
                acs_mode=acs_mode,
            )
        if operation == "stop":
            return stop_node(profile, emit_output=False)
        stop_node(profile, emit_output=False)
        return start_node(
            profile, force_debug, emit_output=False, known_iso_sha=iso_sha,
            mode=mode,
            acs_mode=acs_mode,
        )

    results: dict[str, dict[str, Any]] = {}
    failures: dict[str, str] = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=min(len(ordered), 16)) as pool:
        pending = {pool.submit(invoke, profile): profile for profile in ordered}
        for future in concurrent.futures.as_completed(pending):
            profile = pending[future]
            try:
                results[profile.node_id] = future.result()
            except (LabError, OSError) as error:
                failures[profile.node_id] = str(error)
    for profile in ordered:
        if profile.node_id in failures:
            print(f"{operation}-failed: {profile.node_id}: {failures[profile.node_id]}")
        elif operation == "stop":
            result = results[profile.node_id]
            verb = "already-stopped" if result["already_stopped"] else "stopped"
            print(f"{verb}: {profile.node_id}")
        else:
            result = results[profile.node_id]
            print(
                f"started: {profile.node_id} qemu_pid={result['qemu_pid']} "
                f"mode={result['mode']} network={result['network_attachment']}"
            )
    if failures:
        raise LabError(
            f"{operation}-all completed with {len(failures)} isolated failure(s)"
        )


def explicit_peer_summary(
    profile: NodeProfile,
    events: list[dict[str, Any]],
    malformed: list[str],
) -> ExplicitPeerSummary:
    valid: set[str] = set()
    for event in events:
        if (
            event.get("record") == "acs_signal_validation_result"
            and event.get("local") == profile.node_id
            and event.get("outcome") == "valid"
            and event.get("peer") in profile.acs_peers
        ):
            valid.add(str(event["peer"]))
    missing = [peer for peer in profile.acs_peers if peer not in valid]
    if malformed:
        result = "malformed_evidence"
    elif missing:
        result = "incomplete:" + ",".join(missing)
    else:
        result = "complete"
    return ExplicitPeerSummary(frozenset(valid), len(profile.acs_peers), result)


def discovery_observation_summary(
    node_id: str,
    events: list[dict[str, Any]],
    malformed: list[str],
    expected_remote: set[str],
) -> DiscoveryObservationSummary:
    if malformed:
        return DiscoveryObservationSummary(
            None, None, None, None, "malformed_evidence"
        )

    ready = False
    observations: dict[str, tuple[str, str]] = {}
    for event in events:
        if event.get("local") != node_id:
            continue
        record = event.get("record")
        if record == "acs_discovery_ready" and event.get("outcome") == "ready":
            ready = True
            continue
        peer = event.get("peer")
        if (
            not isinstance(peer, str)
            or NODE_ID_PATTERN.fullmatch(peer) is None
            or peer == node_id
        ):
            continue
        if record == "acs_participant_observed":
            observations[peer] = ("direct", "observed")
        elif record == "acs_participant_hint_received":
            current = observations.get(peer)
            if current is None or current[0] != "direct":
                observations[peer] = ("hint", "observed")
        elif record == "acs_participant_stale":
            kind = observations.get(peer, ("unknown", "observed"))[0]
            observations[peer] = (kind, "stale")
        elif record == "acs_participant_rediscovered":
            observations[peer] = ("direct", "observed")
        elif record == "acs_discovery_conflict":
            kind = observations.get(peer, ("unknown", "observed"))[0]
            observations[peer] = (kind, "conflict")

    if not ready:
        return DiscoveryObservationSummary(
            None, None, None, None, "not_observed"
        )

    direct_peers = {
        peer for peer, (kind, state) in observations.items()
        if kind == "direct" and state == "observed"
    }
    direct = len(direct_peers)
    hint = sum(
        kind == "hint" and state == "observed"
        for kind, state in observations.values()
    )
    stale = sum(state == "stale" for _, state in observations.values())
    conflict = sum(state == "conflict" for _, state in observations.values())
    if stale or conflict:
        result = "degraded"
    elif direct_peers == expected_remote:
        result = "complete"
    else:
        result = "incomplete"
    return DiscoveryObservationSummary(direct, hint, stale, conflict, result)


def peer_validation_state(profile: NodeProfile) -> tuple[set[str], list[str]]:
    events, malformed = read_events(instance_paths(profile))
    summary = explicit_peer_summary(profile, events, malformed)
    return set(summary.valid), malformed


def print_group_status() -> None:
    profiles = load_profiles()
    rows: list[dict[str, str]] = []
    for node_id in sorted(profiles):
        profile = profiles[node_id]
        paths = instance_paths(profile)
        with InstanceLock(paths):
            state, info, _ = reconcile(profile, paths)
            try:
                events, malformed = read_events(paths)
            except LabError:
                events, malformed = [], ["unavailable"]
        selected_mode = str((state or {}).get("acs_mode", profile.acs_mode))
        if selected_mode not in ("explicit", "discovery"):
            selected_mode = profile.acs_mode
        row = {
            "node_id": node_id,
            "state": "running" if info else "stopped",
            "network": network_attachment(profile, info),
            "mode": selected_mode,
        }
        if selected_mode == "explicit":
            summary = explicit_peer_summary(profile, events, malformed)
            row.update({
                "valid": f"{len(summary.valid)}/{summary.expected}",
                "result": summary.result,
            })
        else:
            summary = discovery_observation_summary(
                node_id, events, malformed, set(profiles) - {node_id}
            )
            row.update({
                "direct": "not_observed" if summary.direct is None else str(summary.direct),
                "hint": "not_observed" if summary.hint is None else str(summary.hint),
                "stale": "not_observed" if summary.stale is None else str(summary.stale),
                "conflict": (
                    "not_observed" if summary.conflict is None else str(summary.conflict)
                ),
                "result": summary.result,
            })
        rows.append(row)

    modes = {row["mode"] for row in rows}
    complete = all(row["result"] == "complete" for row in rows)
    if modes == {"explicit"}:
        print("NODE ID\tSTATE\tNETWORK\tVALID PEERS\tACS RESULT")
        for row in rows:
            print(
                f"{row['node_id']}\t{row['state']}\t{row['network']}\t"
                f"{row['valid']}\t{row['result']}"
            )
        print(f"group_acs_exchange: {'complete' if complete else 'incomplete'}")
    elif modes == {"discovery"}:
        print(
            "NODE ID\tSTATE\tNETWORK\tACS MODE\tDIRECT\tHINT\tSTALE\t"
            "CONFLICT\tDISCOVERY RESULT"
        )
        for row in rows:
            print(
                f"{row['node_id']}\t{row['state']}\t{row['network']}\t"
                f"{row['mode']}\t{row['direct']}\t{row['hint']}\t"
                f"{row['stale']}\t{row['conflict']}\t{row['result']}"
            )
        print(f"group_acs_discovery: {'complete' if complete else 'incomplete'}")
    else:
        print("NODE ID\tSTATE\tNETWORK\tACS MODE\tEVIDENCE\tACS RESULT")
        for row in rows:
            evidence = (
                f"valid={row['valid']}"
                if row["mode"] == "explicit"
                else (
                    f"direct={row['direct']},hint={row['hint']},"
                    f"stale={row['stale']},conflict={row['conflict']}"
                )
            )
            print(
                f"{row['node_id']}\t{row['state']}\t{row['network']}\t"
                f"{row['mode']}\t{evidence}\t{row['result']}"
            )
        print(f"group_acs_status: {'complete' if complete else 'incomplete'}")


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


def print_discovery(profile: NodeProfile) -> None:
    events, malformed = read_events(instance_paths(profile))
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")
    selected = [
        event for event in events
        if event.get("record") in {
            "acs_discovery_started",
            "acs_discovery_ready",
            "acs_participant_observed",
            "acs_participant_hint_received",
            "acs_participant_stale",
            "acs_participant_rediscovered",
            "acs_discovery_conflict",
            "acs_discovery_capacity_rejected",
            "acs_discovery_summary",
        }
    ]
    if not selected:
        raise LabError("ACS discovery evidence is not available")
    for event in selected:
        print(json.dumps(event, separators=(",", ":"), sort_keys=True))


def cpu_evidence_summary(profile: NodeProfile) -> dict[str, str]:
    paths = instance_paths(profile)
    events, malformed = read_events(paths)
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")
    selected = {
        "topology": None,
        "capability": None,
        "profile": None,
        "cpu_runtime": None,
        "gpu_runtime": None,
        "result": None,
    }
    runtime_resident: dict[str, Any] | None = None
    runtime_probe: dict[str, Any] | None = None
    for event in events:
        record = event.get("record")
        if (record == "cpu_runtime_resident" and
                event.get("subject") == "node.cpu.runtime" and
                event.get("outcome") == "active"):
            runtime_resident = event
        elif (record == "runtime_post_transition_probe" and
              event.get("subject") == "node.cpu.runtime"):
            runtime_probe = event
        if event.get("subject") != profile.node_id:
            continue
        if record == "cpu_topology_observed":
            selected["topology"] = event
        elif record == "cpu_capability_observed":
            selected["capability"] = event
        elif record == "cpu_profile_evaluation":
            selected["profile"] = event
        elif record == "component_requirement":
            component = event.get("component")
            if component == "cpu.runtime":
                selected["cpu_runtime"] = event
            elif component == "gpu.runtime":
                selected["gpu_runtime"] = event
        elif record == "assembly_requirement_result":
            selected["result"] = event
    if not all(selected.values()):
        missing = ",".join(key for key, value in selected.items() if value is None)
        raise LabError(f"CPU assembly evidence is incomplete for {profile.node_id}: {missing}")

    def field(section: str, key: str) -> str:
        value = selected[section].get(key, "unknown")  # type: ignore[union-attr]
        if isinstance(value, bool):
            return str(value).lower()
        if isinstance(value, (str, int)):
            return str(value)[:128]
        return "unknown"

    def runtime_field(key: str) -> str:
        for event in (runtime_probe, runtime_resident):
            if event is not None and key in event:
                value = event[key]
                if isinstance(value, bool):
                    return str(value).lower()
                if isinstance(value, (str, int)):
                    return str(value)[:128]
        return "not_observed"

    configured = field("topology", "configured")
    online = field("topology", "online")
    allowed = field("topology", "process_allowed")
    try:
        count_consistency = str(cpu_discovery_counts_consistent(
            profile.vcpus, int(configured), int(online), int(allowed)
        )).lower()
    except ValueError:
        count_consistency = "unknown"

    return {
        "node_id": profile.node_id,
        "profile_sockets": str(profile.cpu_sockets),
        "profile_cores": str(profile.cpu_cores),
        "profile_threads": str(profile.cpu_threads),
        "profile_vcpus": str(profile.vcpus),
        "topology_observation": field("topology", "outcome"),
        "configured_logical_processors": configured,
        "online_logical_processors": online,
        "process_allowed_logical_processors": allowed,
        "discovery_counts_consistent": count_consistency,
        "packages": field("topology", "packages"),
        "physical_cores": field("topology", "physical_cores"),
        "numa_nodes": field("topology", "numa_nodes"),
        "smt": field("topology", "smt"),
        "capability_observation": field("capability", "outcome"),
        "architecture": field("capability", "architecture"),
        "vendor": field("capability", "vendor"),
        "pointer_bits": field("capability", "pointer_bits"),
        "observed_simd": field("capability", "common_simd"),
        "atomic_u64": field("capability", "atomic_u64"),
        "selected_profile": field("profile", "profile"),
        "profile_evaluation": field("profile", "outcome"),
        "cpu_runtime": field("cpu_runtime", "outcome"),
        "gpu_runtime": field("gpu_runtime", "outcome"),
        "assembly_requirement_result": field("result", "outcome"),
        "runtime_compiled": field("result", "runtime_compiled"),
        "runtime_activated": field("result", "runtime_activated"),
        "runtime_worker_count": runtime_field("runtime_worker_count"),
        "multi_worker_probe_jobs": runtime_field("multi_worker_probe_jobs"),
        "multi_worker_probe_result": runtime_field("multi_worker_probe_result"),
        "evidence_log": str(paths.event_log),
    }


def print_cpu_info(profile: NodeProfile) -> None:
    summary = cpu_evidence_summary(profile)
    for key, value in summary.items():
        print(f"{key}: {value}")


def print_cpu_summary() -> None:
    profiles = load_profiles()
    print("NODE ID\tPKG\tCORES\tLOGICAL\tSMT\tALLOWED\tWORKERS\tPROFILE\tPROBE")
    incomplete = False
    for node_id in sorted(profiles):
        profile = profiles[node_id]
        try:
            summary = cpu_evidence_summary(profile)
            print(
                f"{node_id}\t{summary['packages']}\t{summary['physical_cores']}\t"
                f"{summary['configured_logical_processors']}\t{summary['smt']}\t"
                f"{summary['process_allowed_logical_processors']}\t"
                f"{summary['runtime_worker_count']}\t{summary['selected_profile']}\t"
                f"{summary['multi_worker_probe_result']}"
            )
        except LabError as error:
            incomplete = True
            print(f"{node_id}\tnot_observed\t-\t-\t-\t-\t-\t-\t{error}")
    if incomplete:
        raise LabError("one or more managed nodes lack complete CPU evidence")


def cpu_runtime_evidence_summary(profile: NodeProfile) -> dict[str, str]:
    paths = instance_paths(profile)
    events, malformed = read_events(paths)
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")
    expected = {
        "request": "cpu_runtime_build_requested",
        "build": "cpu_runtime_build_result",
        "artifact": "cpu_runtime_artifact_created",
        "validation": "cpu_runtime_artifact_validation",
        "acceptance": "cpu_runtime_boot_acceptance",
        "activation": "cpu_runtime_activation_result",
        "probe": "cpu_runtime_probe_result",
        "phase": "cpu_runtime_phase_result",
    }
    selected: dict[str, dict[str, Any] | None] = {
        key: None for key in expected
    }
    by_record = {record: key for key, record in expected.items()}
    for event in events:
        if event.get("subject") != profile.node_id:
            continue
        key = by_record.get(event.get("record"))
        if key is not None:
            selected[key] = event
    if not all(selected.values()):
        missing = ",".join(key for key, value in selected.items() if value is None)
        raise LabError(
            f"CPU runtime evidence is incomplete for {profile.node_id}: {missing}"
        )

    def field(section: str, key: str) -> str:
        event = selected[section]
        value = event.get(key, "unknown") if event is not None else "unknown"
        if isinstance(value, bool):
            return str(value).lower()
        if isinstance(value, (str, int)):
            return str(value)[:128]
        return "unknown"

    return {
        "node_id": profile.node_id,
        "profile": field("request", "profile"),
        "build_request": field("request", "outcome"),
        "build": field("build", "outcome"),
        "compiled": field("phase", "runtime_compiled"),
        "compiled_this_boot": field("build", "compiled_this_boot"),
        "artifact_identity": field("artifact", "artifact_identity"),
        "artifact_sha256": field("artifact", "sha256"),
        "artifact_size_bytes": field("artifact", "size_bytes"),
        "validation": field("validation", "outcome"),
        "boot_acceptance": field("acceptance", "outcome"),
        "activated": field("phase", "runtime_activated"),
        "activation": field("activation", "outcome"),
        "probe": field("probe", "outcome"),
        "probe_result": field("probe", "result"),
        "phase": field("phase", "outcome"),
        "failure_category": field("phase", "failure_category"),
        "global_runtime_ready": field("phase", "global_runtime_ready"),
        "evidence_log": str(paths.event_log),
    }


def print_cpu_runtime(profile: NodeProfile) -> None:
    summary = cpu_runtime_evidence_summary(profile)
    for key, value in summary.items():
        print(f"{key}: {value}")


def print_cpu_runtime_summary() -> None:
    profiles = load_profiles()
    print("NODE ID\tPROFILE\tBUILD\tVALIDATE\tACTIVE\tPROBE\tPHASE")
    incomplete = False
    for node_id in sorted(profiles):
        profile = profiles[node_id]
        try:
            summary = cpu_runtime_evidence_summary(profile)
            print(
                f"{node_id}\t{summary['profile']}\t{summary['build']}\t"
                f"{summary['validation']}\t{summary['activated']}\t"
                f"{summary['probe']}\t{summary['phase']}"
            )
        except LabError as error:
            incomplete = True
            print(f"{node_id}\tnot_observed\t-\t-\t-\t-\t{error}")
    if incomplete:
        raise LabError("one or more managed nodes lack complete CPU runtime evidence")


def resident_runtime_summary(profile: NodeProfile) -> dict[str, str]:
    paths = instance_paths(profile)
    events, malformed = read_events(paths)
    if malformed:
        raise LabError(f"event log contains {len(malformed)} malformed record(s)")
    latest: dict[str, dict[str, Any]] = {}
    observed_records = {
        "runtime_mode_selection",
        "runtime_transition_evaluation",
        "runtime_ready",
        "resident_supervision_entered",
        "cpu_runtime_resident",
        "acs_resident",
        "runtime_post_transition_probe",
        "resident_supervision_exited",
    }
    for event in events:
        record = event.get("record")
        if isinstance(record, str) and record in observed_records:
            latest[record] = event

    def outcome(record: str) -> str:
        value = latest.get(record, {}).get("outcome", "not_observed")
        return str(value)[:128]

    with InstanceLock(paths):
        state, process, _ = reconcile(profile, paths)
    return {
        "node_id": profile.node_id,
        "vm": "running" if process is not None else "stopped",
        "mode": str((state or {}).get("mode", outcome("runtime_mode_selection")))[:32],
        "transition": outcome("runtime_transition_evaluation"),
        "runtime_ready": outcome("runtime_ready"),
        "resident": outcome("resident_supervision_entered"),
        "cpu": outcome("cpu_runtime_resident"),
        "acs": outcome("acs_resident"),
        "post_probe": outcome("runtime_post_transition_probe"),
        "resident_exit": outcome("resident_supervision_exited"),
        "event_source": str(
            paths.event_log if paths.event_log.exists() else paths.serial_log
        ),
    }


def print_runtime_status(profile: NodeProfile) -> None:
    summary = resident_runtime_summary(profile)
    for key, value in summary.items():
        print(f"{key}: {value}")


def print_runtime_summary() -> None:
    profiles = load_profiles()
    print("NODE ID\tVM\tMODE\tTRANSITION\tRUNTIME\tCPU\tACS\tPOST-PROBE")
    incomplete = False
    for node_id in sorted(profiles):
        summary = resident_runtime_summary(profiles[node_id])
        print(
            f"{node_id}\t{summary['vm']}\t{summary['mode']}\t"
            f"{summary['transition']}\t{summary['runtime_ready']}\t"
            f"{summary['cpu']}\t{summary['acs']}\t{summary['post_probe']}"
        )
        incomplete = incomplete or not (
            summary["vm"] == "running"
            and summary["mode"] == "lab"
            and summary["transition"] == "accepted"
            and summary["runtime_ready"] == "ready"
            and summary["resident"] == "resident"
            and summary["cpu"] == "active"
            and summary["acs"] == "active"
            and summary["post_probe"] == "passed"
        )
    if incomplete:
        raise LabError("one or more managed nodes are not resident and runtime-ready")


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
    commands.add_parser("group-status", help="summarize bounded managed-fleet ACS evidence")
    commands.add_parser("cpu-summary", help="summarize managed-fleet CPU decisions")
    commands.add_parser(
        "cpu-runtime-summary", help="summarize managed-fleet CPU runtime evidence"
    )
    commands.add_parser(
        "runtime-summary", help="summarize managed-fleet resident runtime evidence"
    )
    commands.add_parser("stop-all", help="concurrently stop every managed node")
    start_all = commands.add_parser("start-all", help="concurrently start every managed node")
    start_all.add_argument("--debug", action="store_true")
    start_all.add_argument(
        "--mode", choices=("conformance", "lab"), default="conformance"
    )
    start_all.add_argument("--acs-mode", choices=("explicit", "discovery"))
    restart_all = commands.add_parser(
        "restart-all", help="concurrently restart every managed node"
    )
    restart_all.add_argument("--debug", action="store_true")
    restart_all.add_argument(
        "--mode", choices=("conformance", "lab"), default="conformance"
    )
    restart_all.add_argument("--acs-mode", choices=("explicit", "discovery"))
    for name in (
        "status", "stop", "debug-info", "cpu-info", "cpu-runtime",
        "runtime-status",
    ):
        command = commands.add_parser(name)
        command.add_argument("node_id")
    for name in ("start", "restart"):
        command = commands.add_parser(name)
        command.add_argument("node_id")
        command.add_argument("--debug", action="store_true")
        command.add_argument(
            "--mode", choices=("conformance", "lab"), default="conformance"
        )
        command.add_argument("--acs-mode", choices=("explicit", "discovery"))
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
    discovery = commands.add_parser("discovery")
    discovery.add_argument("node_id")
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
        if args.command == "group-status":
            print_group_status()
            return 0
        if args.command == "cpu-summary":
            print_cpu_summary()
            return 0
        if args.command == "cpu-runtime-summary":
            print_cpu_runtime_summary()
            return 0
        if args.command == "runtime-summary":
            print_runtime_summary()
            return 0
        if args.command == "start-all":
            run_group_operation("start", args.debug, args.mode, args.acs_mode)
            return 0
        if args.command == "stop-all":
            run_group_operation("stop")
            return 0
        if args.command == "restart-all":
            run_group_operation("restart", args.debug, args.mode, args.acs_mode)
            return 0
        profile = select_profile(args.node_id)
        if args.command == "status":
            print_status(profile)
        elif args.command == "start":
            start_node(profile, args.debug, mode=args.mode, acs_mode=args.acs_mode)
        elif args.command == "stop":
            stop_node(profile)
        elif args.command == "restart":
            stop_node(profile, allow_stopped=True)
            start_node(profile, args.debug, mode=args.mode, acs_mode=args.acs_mode)
        elif args.command == "events":
            print_events(profile, args.limit)
        elif args.command == "acs-events":
            print_acs_events(profile, args.limit)
        elif args.command == "discovery":
            print_discovery(profile)
        elif args.command == "cpu-info":
            print_cpu_info(profile)
        elif args.command == "cpu-runtime":
            print_cpu_runtime(profile)
        elif args.command == "runtime-status":
            print_runtime_status(profile)
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
