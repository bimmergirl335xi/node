#ifndef SRC_BACKENDS_CUDA_CUDA_EVIDENCE_ADAPTER_HPP
#define SRC_BACKENDS_CUDA_CUDA_EVIDENCE_ADAPTER_HPP

#include <vector>

#include "cuda_backend.hpp"
#include "gpu_evidence_correlation.hpp"

namespace prometheus::backends::cuda {

struct CudaGpuEvidenceCollectionOptions {
    CudaDevicePoolBuildOptions pool_options{};
    hardware::GpuEvidenceCorrelationOptions correlation_options{};
};

struct CudaGpuEvidenceCollectionResult {
    CudaSoftwareVersions software{};
    CudaDevicePoolBuildResult pool{};
    hardware::GpuEvidenceInput evidence{};
    hardware::GpuEvidenceCorrelationResult correlation{};
};

// Maps already-observed CUDA evidence into the CUDA-free correlation model.
// Empty execution_reports means kernel coverage and execution were not
// evaluated; it does not mean unavailable or not ready.
[[nodiscard]] hardware::GpuEvidenceInput adapt_cuda_gpu_evidence(
    const hardware::LinuxPciInventoryResult& hardware_inventory,
    const CudaSoftwareVersions& software,
    const CudaDeviceDiscoveryResult& discovery,
    const CudaDevicePool* pool = nullptr,
    const std::vector<CudaDeviceExecutionReport>& execution_reports = {});

// Observes existing inventory/discovery APIs and correlates their immutable
// results. It does not mutate a backend or registry, perform admission, launch
// kernels, allocate device memory, or authorize execution.
[[nodiscard]] CudaGpuEvidenceCollectionResult collect_cuda_gpu_evidence(
    const hardware::LinuxPciInventoryResult& hardware_inventory,
    const CudaGpuEvidenceCollectionOptions& options = {});

}  // namespace prometheus::backends::cuda

#endif  // SRC_BACKENDS_CUDA_CUDA_EVIDENCE_ADAPTER_HPP
