# AGENTS.md

Repository-wide instructions for AI coding agents working on Node.

## Load context narrowly

Do not recursively read the repository. Use this order:

1. `AGENTS.md`
2. `docs/PROJECT_TREE.md`
3. `docs/CURRENT_STATE.md`
4. `AI_CONTEXT.md` when architectural context is needed
5. the relevant subsystem README or architecture specification
6. only the source and tests needed for the task

`docs/PROJECT_TREE.md` is the navigation authority.
`docs/CURRENT_STATE.md` is the integrated-state authority. Historical phase,
handoff, and validation notes do not override current-state documentation.

## Branch rules

- `main` is the stable/public branch and should represent a validated buildable
  and bootable state for its supported profile.
- `dev` is the active integration branch. It may contain incomplete or
  temporarily non-building work.
- Substantial subsystem work belongs on `lane/cpu`, `lane/gpu`, `lane/apu`,
  `lane/phi`, `lane/runtime`, or `lane/docs` before integration into `dev`.
- `lane/tmp` is preservation/quarantine only.
- Normal promotion is `specialized lane -> dev -> validated main`.

Do not merge branches, rewrite history, force-push, rebase shared branches, or
delete branches unless the operator explicitly requests it.

## Repository routing

- boot, micro-OS, images, profiles: `assembly/`
- runtime and hardware backends: `src/`
- public ABI: `interfaces/`
- tests: `tests/` and subsystem test directories
- developer tools: `tools/`, `scripts/`, `python/`
- public architecture: `docs/architecture/`
- generated output: `build/`
- current production robot behavior: `legacy/vision_swarm_11.cu`

Use `docs/PROJECT_TREE.md` for detailed task routing.

## Protected areas and source safety

Do not:

- delete or rewrite unknown source, tests, interfaces, user data, or unique
  architecture documentation;
- refactor unrelated implementation to simplify a task;
- silently change public ABI or runtime behavior;
- treat a placeholder, proposal, Draft specification, or branch-local change as
  integrated implementation;
- move private ACS code or deployment secrets into the public repository;
- require CUDA, a GPU, a GUI, systemd, desktop services, Raspberry Pi, or a
  particular accelerator for core CPU-only behavior;
- perform destructive provisioning, media writes, installation, activation, or
  control transfer without explicit operator authorization.

The Linux kernel source is external. Do not vendor it into this repository.

## Disposable areas and cleanup

`build/` is generated, ignored, non-authoritative, and safe to remove. Build
trees, CMake output, object files, ISO staging, VM/debug state, logs, validation
output, caches, and obsolete generated checkpoints must not become permanent
source history.

Delete only clearly generated or obsolete material. If a file may contain
unique user or project information, leave it in place and report it.

## Documentation policy

- `README.md` is the public project overview.
- `AGENTS.md` tells agents how to work here.
- `AI_CONTEXT.md` holds durable technical context and invariants.
- `docs/CURRENT_STATE.md` describes what is integrated now.
- `docs/PROJECT_TREE.md` routes readers to the owning subsystem.
- subsystem README files describe their directory, not a development phase.
- architecture specifications belong in `docs/architecture/`.

Do not create milestone or phase README files. Consolidate useful current facts
into the authoritative document, retain genuinely historical material only in
an appropriate documentation area, and remove empty/generated duplicates.
Clearly label implemented, validated, experimental, and planned work.

Update `docs/CURRENT_STATE.md` when integrated technical state changes and
`docs/PROJECT_TREE.md` when navigation changes.

## Hardware and backend rules

Node must remain headless-capable, hardware-agnostic where practical, and
usable without CUDA or a GPU.

Keep these states distinct:

```text
hardware observed
driver bound
runtime available
backend supported
backend ready
admitted for execution
```

Unknown or unavailable evidence must remain explicit; it is not healthy or
ready by default. ARM discovery enriches the generic CPU backend and must not
register the same physical CPU twice. Stable hardware identity must not depend
only on CUDA ordinals, logical CPU numbers, discovery order, or transient device
paths.

Supported ownership models may be Linux-owned, ACS-owned, or coordinated.
Prefer mature Linux drivers unless an explicit architecture requires direct
ownership.

## ACS public/private boundary

Public Node may contain ACS interfaces, ABI definitions, Linux-facing
integration, transport mechanisms, and public runtime integration. Private ACS
implementation, deployment policy, topology, trust decisions, credentials, and
proprietary behavior must remain outside this repository.

Discovery never grants identity, trust, admission, authority, activation, or
readiness. Public ACS admission is currently advisory and non-reserving.

## Testing and reporting

Match validation to the change. Where relevant, consider CPU-only builds,
CUDA-enabled builds, unit/integration tests, image construction, QEMU boot,
failure paths, strict warnings, and `git diff --check`.

Never claim a build, test, benchmark, or boot passed unless it ran in the current
work. Report results as `passed`, `failed`, `not run`, `unavailable`, or
`indeterminate`, and distinguish current runs from historical records.

Keep normal development non-interactive and headless. Human-readable probes are
diagnostic surfaces, not required core behavior.

## Working method

Before changing files, identify the owning subsystem, inspect its existing
implementation and focused tests, and choose the smallest coherent change.
Prefer extending established architecture over adding parallel wrappers or
duplicate documentation. Preserve unrelated work in a dirty checkout.

After a change, inspect the final diff for accidental implementation or ABI
changes, remove disposable outputs, update the authoritative documentation, and
report validation and unresolved issues precisely.

## Instruction authority

Use this precedence when instructions conflict:

```text
explicit operator instruction
    -> AGENTS.md
    -> docs/CURRENT_STATE.md
    -> applicable architecture specification
    -> subsystem README
    -> historical documentation
```

Report unresolved conflicts instead of silently choosing an interpretation.
