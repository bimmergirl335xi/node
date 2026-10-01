#!/usr/bin/env python3
"""Focused validation for managed-node CPU topology policy."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path


PYTHON_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PYTHON_ROOT))

import node_lab  # noqa: E402


class CpuTopologyTests(unittest.TestCase):
    def test_canonical_twenty_five_node_matrix(self):
        profiles = node_lab.load_profiles()
        observed = {
            node_id: (
                item.cpu_sockets, item.cpu_cores, item.cpu_threads, item.vcpus
            )
            for node_id, item in profiles.items()
        }
        expected = {
            "node-001": (1, 1, 1, 1),
            "node-002": (1, 2, 1, 2),
            "node-003": (1, 4, 1, 4),
            "node-004": (1, 2, 2, 4),
            "node-005": (2, 2, 1, 4),
        }
        expected.update({
            f"node-{index:03d}": (1, 1, 1, 1)
            for index in range(6, 26)
        })
        self.assertEqual(observed, expected)

    def test_scale_nodes_use_discovery_with_bounded_memory(self):
        profiles = node_lab.load_profiles()
        for index in range(6, 26):
            item = profiles[f"node-{index:03d}"]
            self.assertEqual(item.memory_mb, 256)
            self.assertEqual(item.acs_mode, "discovery")
            self.assertEqual(item.acs_peers, ())

    def test_original_explicit_peer_sets_remain_available(self):
        profiles = node_lab.load_profiles()
        original = {f"node-{index:03d}" for index in range(1, 6)}
        for index in range(1, 6):
            item = profiles[f"node-{index:03d}"]
            self.assertEqual(item.acs_mode, "discovery")
            self.assertEqual(set(item.acs_peers), original - {item.node_id})

    def test_zero_field_is_rejected(self):
        with self.assertRaisesRegex(node_lab.LabError, "cpu_sockets"):
            node_lab.validate_cpu_topology(1, 0, 1, 1)

    def test_unreasonable_product_is_rejected_before_use(self):
        with self.assertRaisesRegex(node_lab.LabError, "product exceeds"):
            node_lab.validate_cpu_topology(16, 16, 16, 16)

    def test_profile_product_mismatch_is_rejected(self):
        with self.assertRaisesRegex(node_lab.LabError, "must equal"):
            node_lab.validate_cpu_topology(4, 1, 2, 1)

    def test_discovery_count_consistency(self):
        self.assertTrue(node_lab.cpu_discovery_counts_consistent(4, 4, 4, 4))
        self.assertTrue(node_lab.cpu_discovery_counts_consistent(4, 4, 4, 2))
        self.assertFalse(node_lab.cpu_discovery_counts_consistent(4, 2, 2, 2))
        self.assertFalse(node_lab.cpu_discovery_counts_consistent(4, 4, 2, 3))


if __name__ == "__main__":
    unittest.main()
