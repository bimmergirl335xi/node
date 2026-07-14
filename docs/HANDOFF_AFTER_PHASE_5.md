# Chat Handoff After Phase 5

Use this checklist when starting the next development chat.

## Attach or Paste

- `AI_CONTEXT.md`
- `docs/CURRENT_STATE.md`
- `src/kernels/learning_kernels.cuh`
- `src/kernels/learning_kernels.cu`
- `src/kernels/CMakeLists.txt`
- root `CMakeLists.txt`
- Phase 5 test output
- Phase 5 benchmark output
- any compiler warnings or errors
- focused Git diff for Phase 5

## Opening Message

A suitable opening message is:

> We completed Prometheus kernel phases 1 through 5. The tests pass. Please read
> AI_CONTEXT.md and docs/CURRENT_STATE.md first. We are starting Phase 6:
> kernel registry and per-device dispatch. Preserve the existing robot
> behavior and do not treat placeholder files as implemented.

## Planned Next Five Phases

6. CUDA kernel registry and per-device dispatch
7. CPU backend equivalents and reference operations
8. Dynamic CUDA device backend
9. Visual perception worker extraction
10. Local message bus integration
