#!/usr/bin/env python3
import json
import re
import sys
from pathlib import Path


def fail(message: str) -> None:
    raise SystemExit(f"NODE_P0 RECORD ERROR {message}")


def load(records: Path, name: str) -> dict:
    path = records / name
    try:
        raw = path.read_bytes()
        if not raw or len(raw) > 32768:
            fail(f"invalid record size: {name}")
        value = json.loads(raw)
    except (OSError, json.JSONDecodeError) as error:
        fail(f"invalid JSON record {name}: {error}")
    if not isinstance(value, dict):
        fail(f"record is not an object: {name}")
    return value


def require_keys(value: dict, keys: set[str], name: str) -> None:
    missing = keys.difference(value)
    if missing:
        fail(f"{name} lacks fields: {sorted(missing)}")


if len(sys.argv) != 2:
    fail("usage: validate_provider_records.py RECORDS_DIR")
records = Path(sys.argv[1])

contract = load(records, "p0-conformance-authority-v1.json")
operation = load(records, "p0-conformance-operation.json")
provider = load(records, "asm-provider-operation.json")
candidates = [
    load(records, "candidate-kernel-output.json"),
    load(records, "candidate-initramfs-output.json"),
]

if contract.get("contract_identity") != "node.ram-assembly-p0.conformance-authority":
    fail("unexpected authority contract identity")
if contract.get("contract_revision") != 1:
    fail("unexpected authority contract revision")
if contract.get("authority_state") != "AUTHORITY_NOT_REQUIRED":
    fail("authority contract is not AUTHORITY_NOT_REQUIRED")
for required_absence in {
    "boot_recovery_plan",
    "boot_semantic_operation",
    "accepted_krn_artifact",
    "boot_scoped_artifact_acceptance",
    "assembly_generation",
    "installed_instance",
    "activation_result",
    "recovery_result",
    "runtime_readiness_result",
}:
    if required_absence not in contract.get("semantic_absences", []):
        fail(f"contract omits semantic absence: {required_absence}")

identity_keys = {
    "p0_conformance_operation_identity",
    "asm_provider_operation_identity",
    "asm_provider_attempt_identity",
}
require_keys(operation, identity_keys | {"authority_state", "cleanup_state"}, "operation")
identities = [operation[key] for key in identity_keys]
if len(set(identities)) != 3:
    fail("operation and attempt identities are not distinct")
if not all(re.fullmatch(r"[A-Za-z0-9._-]{1,128}", value) for value in identities):
    fail("operation identity is not bounded")

require_keys(
    provider,
    identity_keys
    | {
        "provider_identity",
        "provider_revision",
        "toolchain_identity",
        "reviewed_kernel_source_revision",
        "configuration_references",
        "resource_limits",
        "resource_outcome",
        "provider_outcome",
        "candidate_output_references",
        "cleanup_state",
        "unresolved_effects",
        "limitations",
    },
    "provider operation",
)
for key in identity_keys:
    if provider[key] != operation[key]:
        fail(f"provider correlation mismatch: {key}")
if not re.fullmatch(r"[0-9a-f]{40,64}", provider["reviewed_kernel_source_revision"]):
    fail("reviewed source revision is invalid")
limits = provider["resource_limits"]
outcome = provider["resource_outcome"]
if not isinstance(limits, dict) or limits.get("kernel_build_jobs") != 1:
    fail("provider resource limits are invalid")
if not isinstance(outcome, dict) or outcome.get("active_swap_entries") != 0:
    fail("provider resource outcome is invalid")

expected_classes = {"asm_candidate_kernel_image", "asm_candidate_initramfs_image"}
observed_classes: set[str] = set()
for candidate in candidates:
    require_keys(
        candidate,
        identity_keys
        | {
            "provider_identity",
            "candidate_output_identity",
            "output_class",
            "output_path_or_handle",
            "digest",
            "size_bytes",
            "provider_validation_state",
            "limitations",
            "cleanup_state",
            "unresolved_effects",
        },
        "candidate output",
    )
    for key in identity_keys:
        if candidate[key] != operation[key]:
            fail(f"candidate correlation mismatch: {key}")
    observed_classes.add(candidate["output_class"])
    digest = candidate["digest"]
    if not isinstance(digest, dict) or digest.get("algorithm") != "sha256":
        fail("candidate digest algorithm is invalid")
    if not re.fullmatch(r"[0-9a-f]{64}", digest.get("value", "")):
        fail("candidate digest is invalid")
    if not isinstance(candidate["size_bytes"], int) or candidate["size_bytes"] <= 0:
        fail("candidate size is invalid")
    if candidate["cleanup_state"] != "pending_tmpfs_disposal":
        fail("candidate cleanup state is not explicit")

if observed_classes != expected_classes:
    fail("candidate output classes are incomplete")

prohibited_claim_keys = {
    "krn_artifact_acceptance",
    "boot_scoped_artifact_acceptance",
    "generation_membership",
    "installation_eligibility",
    "activation",
    "recovery",
    "readiness",
}
for name, value in [("provider", provider), ("kernel", candidates[0]), ("initramfs", candidates[1])]:
    if prohibited_claim_keys.intersection(value):
        fail(f"{name} record contains prohibited downstream claims")

print("NODE_P0 RECORD JSON schema and boundary validation passed")
