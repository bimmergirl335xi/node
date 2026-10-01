#!/usr/bin/env python3
"""Focused ACS mode and managed-profile validation tests."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import node_lab  # noqa: E402


def profile(node_id: str, index: int, mode: str, peers: list[str]) -> dict[str, object]:
    return {
        "node_id": node_id,
        "firmware": "bios",
        "vcpus": 1,
        "cpu_sockets": 1,
        "cpu_cores": 1,
        "cpu_threads": 1,
        "memory_mb": 512,
        "debug_enabled": False,
        "gdb_port": 1200 + index,
        "network_enabled": True,
        "mac_address": f"02:00:00:00:00:{index:02x}",
        "ipv4_address": f"10.77.0.{index}/24",
        "acs_port": 39001,
        "network_multicast_address": "230.0.0.1",
        "network_multicast_port": 39000,
        "network_local_address": "127.0.0.1",
        "acs_mode": mode,
        "acs_peers": peers,
    }


class AcsModeTests(unittest.TestCase):
    def load(self, values: list[dict[str, object]]) -> dict[str, node_lab.NodeProfile]:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for value in values:
                path = root / f"{value['node_id']}.json"
                path.write_text(json.dumps(value), encoding="utf-8")
            return node_lab.load_profiles(root)

    def test_explicit_mode_requires_complete_peers(self) -> None:
        values = [
            profile("node-001", 1, "explicit", ["node-002"]),
            profile("node-002", 2, "explicit", []),
        ]
        with self.assertRaisesRegex(node_lab.LabError, "explicit-mode"):
            self.load(values)

    def test_discovery_mode_accepts_zero_and_partial_peers(self) -> None:
        values = [
            profile("node-001", 1, "discovery", []),
            profile("node-002", 2, "discovery", ["node-001"]),
            profile("node-003", 3, "discovery", []),
        ]
        loaded = self.load(values)
        self.assertEqual(loaded["node-001"].acs_peers, ())
        self.assertEqual(loaded["node-002"].acs_peers, ("node-001",))

    def test_invalid_mode_is_rejected(self) -> None:
        values = [profile("node-001", 1, "automatic", [])]
        with self.assertRaisesRegex(node_lab.LabError, "acs_mode"):
            self.load(values)

    def test_network_and_debug_uniqueness_remain_enforced(self) -> None:
        values = [
            profile("node-001", 1, "discovery", []),
            profile("node-002", 2, "discovery", []),
        ]
        values[1]["ipv4_address"] = values[0]["ipv4_address"]
        with self.assertRaisesRegex(node_lab.LabError, "IPv4"):
            self.load(values)
        values[1]["ipv4_address"] = "10.77.0.2/24"
        values[1]["mac_address"] = values[0]["mac_address"]
        with self.assertRaisesRegex(node_lab.LabError, "MAC"):
            self.load(values)
        values[1]["mac_address"] = "02:00:00:00:00:02"
        values[1]["gdb_port"] = values[0]["gdb_port"]
        with self.assertRaisesRegex(node_lab.LabError, "GDB"):
            self.load(values)

    def test_mode_has_deterministic_guest_encoding(self) -> None:
        loaded = self.load([
            profile("node-001", 1, "explicit", ["node-002"]),
            profile("node-002", 2, "explicit", ["node-001"]),
        ])
        explicit = node_lab.smbios_argument(
            loaded["node-001"], loaded, "lab", "explicit"
        )
        self.assertEqual(
            explicit,
            "type=1,manufacturer=node-002@10.77.0.2,"
            "product=Node-Development-VM-lab,version=acs-profile-v1-port-39001,"
            "serial=node-001,sku=10.77.0.1,family=",
        )
        discovery = node_lab.smbios_argument(
            loaded["node-001"], loaded, "lab", "discovery"
        )
        self.assertIn("product=Node-Development-VM-lab", discovery)
        self.assertIn("version=acs-discovery-v1-port-39001", discovery)
        self.assertIn("manufacturer=none", discovery)
        self.assertIn("family=none", discovery)


if __name__ == "__main__":
    unittest.main()
