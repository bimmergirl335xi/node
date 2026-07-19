# Assembly dependency inventory

This directory is the canonical public inventory of tools needed to build and
test the Node source tree and the open assembly environment. It is descriptive:
the repository does not install packages, modify the host, or begin P01 image
construction.

The inventory is split by ownership:

- `host-tools.txt` names commands by build or runtime role;
- `debian-packages.txt` maps those commands to Debian-family package names;
- `kernel-build-tools.txt` records Linux kernel and static-initramfs needs;
- `toolchain-matrix.md` records only version and architecture constraints that
  are enforced or explicitly documented by the current tree.

## Requirement classes

Mandatory public CPU-only build requirements are CMake 3.24 or newer, a C11
compiler, a C++17 compiler, a supported CMake build tool, POSIX threads, and
CTest when tests are enabled. The assembly manifest parser is included in that
ordinary CPU-only build.

CUDA is optional. CUDA-dependent targets require `nvcc`, CUDAToolkit, a
compatible host compiler, and a compatible driver/device only for runtime
tests. A CUDA compiler that is absent while `PROMETHEUS_ENABLE_CUDA=ON` causes
CUDA targets to be disabled, not the CPU build to fail.

RAM Assembly P0 additionally needs the tools listed by its strict preflight,
static C link support, a reviewed Linux source tree or bundle, tmpfs support,
and leaf-operation privilege tools for mounting and kexec loading/execution.
P0 does not install packages and does not require privileged compilation.

P01 USB-image tools are planning inputs only. No image layout, bootloader,
partition table, filesystem, signing workflow, or package set is selected by
this inventory. Candidate tools remain non-mandatory until a later P01
contract chooses the image format and validates the host workflow.

Python has no declared package dependency for this slice. Python 3 is used only
to run repository scripts and P0 JSON record validation, so no generic Python
`requirements.txt` is added here.
