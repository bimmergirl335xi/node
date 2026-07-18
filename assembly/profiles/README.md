# Public assembly profiles

Profiles select replaceable public assembly inputs without embedding
deployment policy. A profile may identify a target architecture, hardware
evidence expectations, kernel configuration fragments, initramfs composition,
and public compatibility versions.

The current concrete fragments live in `../ram_assembly_p0/config/` and include
an adaptable x86_64 baseline plus a Dell Wyse 5070 first-target fragment. The
Wyse name describes a hardware target, not an identity, trust decision, or
universal kernel profile.

Kernel source identity, source provenance, selected revision, configuration,
toolchain, and resulting artifact identity remain separate records. A profile
does not authorize compilation, acceptance, installation, or activation.
