# Toolchain matrix

| Scope | Language/format | Current constraint | Runtime hardware needed | Notes |
| --- | --- | --- | --- | --- |
| Public CPU-only Node build | C11 and C++17 | CMake 3.24 or newer; compiler must support the declared language standards | No | POSIX threads are required. Tests and benchmarks are separately selectable. |
| Public assembly manifest ABI | C11 | Same C compiler and CMake baseline as CPU-only build | No | Built and tested without CUDA. |
| RAM Assembly P0 payloads | strict static C11 | Compiler and libc must support `-static`; warnings are errors | No for host validation | Produces shell-free initramfs payloads. |
| RAM Assembly P0 kernel | x86_64 Linux | GCC/binutils-compatible kernel toolchain; serial build | Dell Wyse hardware only for later physical validation | P0 records exact compiler, linker, and make identities. |
| CUDA Pascal/Volta release | CUDA C++17, SM 6.1/7.0 | CUDA 12.x is the documented family; CUDA 13 or newer is rejected for architectures below SM 7.5 | Compatible NVIDIA driver/device for runtime tests | Default configured architectures are `61;70`. |
| CUDA Turing and newer | CUDA C++17, SM 7.5+ | CUDA 12.x validated; CUDA 13.x may target SM 7.5+ | Compatible NVIDIA driver/device for runtime tests | Device-independent CPU tests do not require the driver. |
| CUDA Kepler release | CUDA C++17, legacy SM below 5.0 | CUDA 11.4 is the documented separate family; CUDA 12 or newer is rejected | Compatible legacy driver/device for runtime tests | Must not be combined with the CUDA 12.x release family. |
| P01 USB image | Not selected | No current toolchain contract | No current claim | Candidate Debian tools are inventoried only; P01 has not begun. |

CUDA runtime validation is distinct from CUDA compilation. A successful CUDA
configure/build does not claim device execution when no compatible device is
available. Likewise, host P0 validation does not claim a physical thin-client
kernel build or kexec control transfer.
