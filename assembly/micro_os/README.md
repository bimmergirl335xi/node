# Node micro-OS boundary

The Node micro-OS begins after Linux kernel handoff and provides the bounded
early-userspace environment needed to start public runtime substrate.

Its PID 1 is responsible for mounting required volatile filesystems, reporting
structured startup results, sequencing public runtime startup, and preserving
a safe failure path. The RAM P0 `init` is only a mechanism proof of that
environment; it is not the complete production micro-OS.

Externally supplied runtime components are separate userspace generation
members, never Linux kernel modules. PID 1 must not discover or execute them
merely because a binary or manifest exists. A future launcher must consume an
explicit accepted-generation description and report launch separately from
readiness.
