# Init sequencing contract

The public startup sequence is ordered:

1. Linux hands control to the selected micro-OS PID 1.
2. PID 1 establishes its bounded volatile runtime environment.
3. PID 1 starts the public Node runtime substrate.
4. The runtime exposes the public ACS substrate from `src/core/acs/`.
5. Only an explicitly accepted generation may request launch of optional
   external components.

Each transition needs its own structured result. Process creation is not
readiness, successful parsing is not acceptance, and failure to start an
optional component must remain bounded and attributable. The current RAM P0
does not implement steps 3 through 5; it proves PID 1 and test-runner
mechanisms without overclaiming normal runtime handoff.
