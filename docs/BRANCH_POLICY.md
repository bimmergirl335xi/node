# Permanent branch policy

The Node repository uses a small permanent branch set. Branch history is
append-only under ordinary operation: integration uses non-force fast-forwards
or merge commits, never rebases, squash merges, force pushes, hard resets, or
history rewrites.

## Approved branches

| Branch | Ownership |
| --- | --- |
| `main` | Deliberately integrated, validated repository checkpoints. |
| `lane/runtime` | Shared runtime composition, public ACS runtime surfaces, assembly and micro-OS implementation, public external-component interfaces, and cross-subsystem runtime integration. |
| `lane/docs` | Public architecture, branch policy, and project-state documentation. |
| `lane/cpu` | CPU and generic ARM implementation, tests, and directly related addenda. |
| `lane/gpu` | GPU/CUDA implementation, tests, and directly related addenda. |
| `lane/apu` | APU implementation, tests, and directly related addenda. |
| `lane/phi` | Intel Phi implementation, tests, and directly related addenda. |
| `lane/codex` | Codex's one continuous bounded-task working branch; never an integration authority. |
| `lane/tmp` | Quarantine for unique history that cannot yet be assigned safely to a domain lane. |

No other long-lived branch is approved. New `agent/*`, `tmp/*`, `feature/*`,
`recovery/*`, `backup/*`, `lane/boot`, and `lane/bootstrap` branches are
prohibited. Open assembly and micro-OS work belongs to `lane/runtime`.

## Codex workflow and cross-domain gate

For a bounded task, `lane/codex` begins from `main`, merges exactly one selected
target lane, performs and validates that lane's work, and merges back into the
same target lane. Completed work must not remain permanently owned by
`lane/codex`.

Before Codex changes target domains, `lane/codex` must be an ancestor of
`main`. If it is not, work may continue only for the same target lane. A mixed
or unresolved `lane/codex` history must never be merged into another subsystem
lane.

## `lane/tmp` quarantine

`lane/tmp` is not a development or main-integration branch. Every preserved
history must have an external reconciliation-ledger entry naming its source
branch and original head, why it could not enter a normal lane, required
review, likely destination, and validation status. Tree-neutral preservation
merges may retain historical reachability without replacing canonical source.
`lane/tmp` must never be merged directly into `main`.

## Integration checkpoints

A lane checkpoint may enter `main` only when its exact source commits are
identified, its target lane is clean, relevant serial builds and tests pass,
unresolved review findings and physical-validation claims are excluded,
documentation matches actual state, and public/private boundaries are checked.
The regular sequence is:

```text
validated lane checkpoint
    -> lane documentation/state update
    -> full integration validation
    -> non-force merge into main
```

Device-dependent, architecture-dependent, and physical-control tests are
reported separately. Host validation must not be described as physical device,
installation, activation, recovery, or readiness validation.

## Documentation, validation, and deletion

Public architecture and shared project-state updates belong to `lane/docs`.
`AI_CONTEXT.md` and `docs/CURRENT_STATE.md` retain append-only checkpoint
evidence unless a separately authorized reconciliation requires a focused
correction. Implementation lanes may carry directly related addenda, but they
do not replace the public documentation authority.

Temporary branches are unnecessary under this model. Existing non-canonical
branches remain evidence until their unique commits are reachable from an
approved lane or explicitly quarantined and their working data is reconciled.
Only the operator deletes branches, refs, worktrees, checkouts, stashes,
untracked files, or file trees after reviewing the external reconciliation
ledger.
