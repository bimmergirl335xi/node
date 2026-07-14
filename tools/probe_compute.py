#!/usr/bin/env python3
"""
Collect a local Prometheus compute inventory without requiring internet access.

This probe does not install drivers or modify the machine. It records evidence
needed to choose a backend and release family, including possible Xeon Phi
Knights Corner or Knights Landing hardware.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
from pathlib import Path
from typing import Any


def run(command: list[str], timeout: float = 8.0) -> dict[str, Any]:
    executable = shutil.which(command[0])
    if executable is None:
        return {
            "available": False,
            "command": command,
        }

    try:
        completed = subprocess.run(
            [executable, *command[1:]],
            check=False,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return {
            "available": True,
            "command": command,
            "error": str(exc),
        }

    return {
        "available": True,
        "command": command,
        "returncode": completed.returncode,
        "stdout": completed.stdout.strip(),
        "stderr": completed.stderr.strip(),
    }


def read_text(path: Path) -> str | None:
    try:
        return path.read_text(encoding="utf-8", errors="replace").strip()
    except OSError:
        return None


def list_directory(path: Path) -> list[str]:
    try:
        return sorted(entry.name for entry in path.iterdir())
    except OSError:
        return []


def detect_phi(
    lscpu_output: str,
    lspci_output: str,
    mic_devices: list[str],
) -> dict[str, Any]:
    combined = f"{lscpu_output}\n{lspci_output}".lower()

    evidence: list[str] = []
    family = "not_detected"

    if mic_devices:
        evidence.append(f"/sys/class/mic entries: {', '.join(mic_devices)}")
        family = "knights_corner_coprocessor_candidate"

    if "xeon phi" in lspci_output.lower() or "many integrated core" in combined:
        evidence.append("PCIe inventory mentions Xeon Phi or MIC")
        if family == "not_detected":
            family = "knights_corner_coprocessor_candidate"

    if "xeon phi" in lscpu_output.lower():
        evidence.append("CPU model identifies as Xeon Phi")
        family = "knights_landing_or_knights_mill_host_candidate"

    return {
        "family": family,
        "evidence": evidence,
        "note": (
            "Confirm the exact model number before selecting a Phi backend. "
            "Knights Corner PCIe coprocessors and bootable Knights Landing/"
            "Knights Mill systems require different runtimes."
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Collect Prometheus compute capability evidence."
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Write JSON to this file instead of stdout.",
    )
    args = parser.parse_args()

    lscpu = run(["lscpu"])
    lspci = run(["lspci", "-nn"])
    nvidia_query = run(
        [
            "nvidia-smi",
            "--query-gpu=index,name,pci.bus_id,driver_version,memory.total",
            "--format=csv,noheader",
        ]
    )
    nvidia_list = run(["nvidia-smi", "-L"])
    nvcc = run(["nvcc", "--version"])
    micinfo = run(["micinfo"])

    lscpu_text = str(lscpu.get("stdout", ""))
    lspci_text = str(lspci.get("stdout", ""))
    mic_devices = list_directory(Path("/sys/class/mic"))

    report = {
        "schema": "prometheus.compute_probe.v1",
        "host": {
            "hostname": platform.node(),
            "platform": platform.platform(),
            "machine": platform.machine(),
            "processor": platform.processor(),
            "python": platform.python_version(),
            "kernel": platform.release(),
        },
        "cpu": {
            "lscpu": lscpu,
            "cpuinfo_head": (
                read_text(Path("/proc/cpuinfo")) or ""
            )[:12000],
        },
        "pci": {
            "lspci": lspci,
        },
        "nvidia": {
            "query": nvidia_query,
            "list": nvidia_list,
            "nvcc": nvcc,
        },
        "xeon_phi": {
            "micinfo": micinfo,
            "sys_class_mic": mic_devices,
            "detection": detect_phi(
                lscpu_text,
                lspci_text,
                mic_devices,
            ),
        },
        "environment": {
            "cuda_visible_devices": os.environ.get(
                "CUDA_VISIBLE_DEVICES"
            ),
            "omp_num_threads": os.environ.get("OMP_NUM_THREADS"),
        },
    }

    serialized = json.dumps(report, indent=2, sort_keys=True)

    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(
            serialized + "\n",
            encoding="utf-8",
        )
        print(args.output)
    else:
        print(serialized)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
