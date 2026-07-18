#include "cuda_kernel_adapter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iterator>
#include <utility>

#include "kernels/activation_kernels.cuh"

namespace prometheus::backends::cuda {
namespace {

[[nodiscard]] bool valid_kernel_id(std::string_view value) noexcept {
    if (value.empty()) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](char character) {
        const unsigned char byte = static_cast<unsigned char>(character);
        return std::isalnum(byte) != 0 || character == '.' ||
               character == '_' || character == '-';
    });
}

[[nodiscard]] std::string bounded_text(
    std::string value,
    std::size_t maximum_bytes) {
    if (value.size() > maximum_bytes) {
        value.resize(maximum_bytes);
    }
    return value;
}

[[nodiscard]] std::string cuda_error_message(
    cudaError_t error,
    const char* operation) {
    std::string message = operation;
    message += " failed";
    const char* name = cudaGetErrorName(error);
    const char* description = cudaGetErrorString(error);
    if (name != nullptr) {
        message += " (";
        message += name;
        message += ')';
    }
    if (description != nullptr) {
        message += ": ";
        message += description;
    }
    return bounded_text(
        std::move(message),
        kCudaAdapterMessageAbsoluteMaximum);
}

}  // namespace

CudaKernelAdapterRegistry::CudaKernelAdapterRegistry(
    CudaKernelAdapterRegistryOptions options)
    : options_(std::move(options)) {
    options_valid_ = options_.maximum_adapters > 0 &&
                     options_.maximum_adapters <=
                         kCudaAdapterCountAbsoluteMaximum &&
                     options_.maximum_identifier_bytes > 0 &&
                     options_.maximum_identifier_bytes <=
                         kCudaAdapterIdentifierAbsoluteMaximum &&
                     options_.maximum_message_bytes > 0 &&
                     options_.maximum_message_bytes <=
                         kCudaAdapterMessageAbsoluteMaximum;
}

CudaKernelAdapterResult CudaKernelAdapterRegistry::register_adapter(
    std::unique_ptr<CudaKernelLaunchAdapter> adapter) noexcept {
    try {
        if (!options_valid_) {
            return bounded_result(
                CudaKernelAdapterCode::invalid_adapter,
                "CUDA adapter registry bounds are invalid");
        }
        if (initialized_) {
            return bounded_result(
                CudaKernelAdapterCode::registry_active,
                "CUDA adapters cannot be registered while initialized");
        }
        if (!adapter) {
            return bounded_result(
                CudaKernelAdapterCode::invalid_adapter,
                "CUDA adapter cannot be null");
        }

        const std::string_view identifier = adapter->kernel_id();
        if (!valid_kernel_id(identifier) ||
            identifier.size() > options_.maximum_identifier_bytes ||
            identifier.size() > kCudaAdapterIdentifierAbsoluteMaximum) {
            return bounded_result(
                CudaKernelAdapterCode::invalid_kernel_id,
                "CUDA adapter kernel ID is invalid or exceeds its bound");
        }
        if (adapter->initialized()) {
            return bounded_result(
                CudaKernelAdapterCode::invalid_adapter,
                "A pre-initialized CUDA adapter cannot be registered");
        }
        if (find(identifier) != nullptr) {
            return bounded_result(
                CudaKernelAdapterCode::duplicate_kernel_id,
                "CUDA adapter kernel ID is already registered");
        }
        if (adapters_.size() >= options_.maximum_adapters) {
            return bounded_result(
                CudaKernelAdapterCode::capacity_reached,
                "CUDA adapter registry capacity was reached");
        }

        adapters_.push_back(std::move(adapter));
        std::sort(
            adapters_.begin(),
            adapters_.end(),
            [](const auto& left, const auto& right) {
                return left->kernel_id() < right->kernel_id();
            });
        return {};
    } catch (...) {
        return {CudaKernelAdapterCode::resource_exhausted};
    }
}

CudaKernelLaunchAdapter* CudaKernelAdapterRegistry::find(
    std::string_view kernel_id) noexcept {
    const auto iterator = std::find_if(
        adapters_.begin(), adapters_.end(), [&](const auto& adapter) {
            return adapter->kernel_id() == kernel_id;
        });
    return iterator == adapters_.end() ? nullptr : iterator->get();
}

const CudaKernelLaunchAdapter* CudaKernelAdapterRegistry::find(
    std::string_view kernel_id) const noexcept {
    const auto iterator = std::find_if(
        adapters_.begin(), adapters_.end(), [&](const auto& adapter) {
            return adapter->kernel_id() == kernel_id;
        });
    return iterator == adapters_.end() ? nullptr : iterator->get();
}

std::vector<CudaKernelAdapterInventoryItem>
CudaKernelAdapterRegistry::inventory() const noexcept {
    try {
        std::vector<CudaKernelAdapterInventoryItem> result{};
        result.reserve(adapters_.size());
        for (const auto& adapter : adapters_) {
            CudaKernelAdapterInventoryItem item{};
            item.kernel_id = adapter->kernel_id();
            item.adapter_name = adapter->adapter_name();
            item.initialized = adapter->initialized();
            result.push_back(std::move(item));
        }
        return result;
    } catch (...) {
        return {};
    }
}

std::size_t CudaKernelAdapterRegistry::size() const noexcept {
    return adapters_.size();
}

bool CudaKernelAdapterRegistry::initialized() const noexcept {
    return initialized_;
}

CudaKernelAdapterResult CudaKernelAdapterRegistry::initialize_all(
    const CudaKernelAdapterDeviceContext& context) noexcept {
    try {
        if (!options_valid_ || context.persistent_device_key.empty() ||
            context.runtime_ordinal_snapshot < 0 || context.stream == nullptr) {
            return bounded_result(
                CudaKernelAdapterCode::invalid_context,
                "CUDA adapter initialization context is invalid",
                0,
                true);
        }
        if (initialized_) {
            return bounded_result(
                CudaKernelAdapterCode::already_initialized,
                "CUDA adapter registry is already initialized",
                0,
                true);
        }

        std::size_t initialized_count = 0;
        for (auto& adapter : adapters_) {
            CudaKernelAdapterResult result = adapter->initialize(context);
            if (!result.ok() || !adapter->initialized()) {
                (void)adapter->shutdown();
                while (initialized_count > 0) {
                    --initialized_count;
                    (void)adapters_[initialized_count]->shutdown();
                }
                if (result.ok()) {
                    result = bounded_result(
                        CudaKernelAdapterCode::initialization_failed,
                        "CUDA adapter did not publish initialized state",
                        0,
                        true);
                }
                result.worker_fatal = true;
                return result;
            }
            ++initialized_count;
        }
        initialized_ = true;
        return {};
    } catch (...) {
        for (auto& adapter : adapters_) {
            (void)adapter->shutdown();
        }
        initialized_ = false;
        return {CudaKernelAdapterCode::resource_exhausted, 0, true};
    }
}

CudaKernelAdapterResult CudaKernelAdapterRegistry::shutdown_all() noexcept {
    CudaKernelAdapterResult first_failure{};
    try {
        for (auto iterator = adapters_.rbegin();
             iterator != adapters_.rend();
             ++iterator) {
            CudaKernelAdapterResult result = (*iterator)->shutdown();
            if (!result.ok() && first_failure.ok()) {
                first_failure = std::move(result);
            }
        }
        initialized_ = false;
        return first_failure;
    } catch (...) {
        initialized_ = false;
        return {CudaKernelAdapterCode::resource_exhausted, 0, true};
    }
}

CudaKernelAdapterResult CudaKernelAdapterRegistry::bounded_result(
    CudaKernelAdapterCode code,
    std::string message,
    int native_error_code,
    bool worker_fatal) const noexcept {
    try {
        return {
            code,
            native_error_code,
            worker_fatal,
            bounded_text(std::move(message), options_.maximum_message_bytes),
        };
    } catch (...) {
        return {code, native_error_code, worker_fatal};
    }
}

std::string_view CudaSyntheticFp32Adapter::kernel_id() const noexcept {
    return kCudaSyntheticExecutionKernelId;
}

std::string_view CudaSyntheticFp32Adapter::adapter_name() const noexcept {
    return "cuda_synthetic_relu_fp32";
}

bool CudaSyntheticFp32Adapter::initialized() const noexcept {
    return initialized_;
}

CudaKernelAdapterResult CudaSyntheticFp32Adapter::initialize(
    const CudaKernelAdapterDeviceContext& context) noexcept {
    try {
        static_assert(
            kCudaSyntheticProbeElementCount <=
                kCudaSyntheticProbeElementAbsoluteMaximum,
            "Synthetic probe element bound exceeded");
        if (initialized_) {
            return {
                CudaKernelAdapterCode::already_initialized,
                0,
                true,
                "Synthetic CUDA adapter is already initialized",
            };
        }
        if (context.persistent_device_key.empty() ||
            context.runtime_ordinal_snapshot < 0 || context.stream == nullptr) {
            return {
                CudaKernelAdapterCode::invalid_context,
                0,
                true,
                "Synthetic CUDA adapter context is invalid",
            };
        }

        constexpr std::array<float, kCudaSyntheticProbeElementCount> kInput{
            -4.0F, -3.0F, -2.0F, -1.0F, 0.0F, 1.0F, 2.0F, 4.0F};
        std::copy(kInput.begin(), kInput.end(), host_input_);
        std::fill(std::begin(host_output_), std::end(host_output_), -99.0F);

        void* allocation = nullptr;
        cudaError_t status = cudaMalloc(
            &allocation,
            sizeof(host_input_));
        if (status != cudaSuccess) {
            return cuda_failure(
                CudaKernelAdapterCode::initialization_failed,
                status,
                "cudaMalloc synthetic input",
                true);
        }
        device_input_ = static_cast<float*>(allocation);

        allocation = nullptr;
        status = cudaMalloc(&allocation, sizeof(host_output_));
        if (status != cudaSuccess) {
            const cudaError_t release_status = cudaFree(device_input_);
            device_input_ = nullptr;
            if (release_status != cudaSuccess) {
                return cuda_failure(
                    CudaKernelAdapterCode::shutdown_failed,
                    release_status,
                    "cudaFree synthetic input after allocation failure",
                    true);
            }
            return cuda_failure(
                CudaKernelAdapterCode::initialization_failed,
                status,
                "cudaMalloc synthetic output",
                true);
        }
        device_output_ = static_cast<float*>(allocation);
        runtime_ordinal_snapshot_ = context.runtime_ordinal_snapshot;
        stream_ = context.stream;
        initialized_ = true;
        launch_pending_validation_ = false;
        return {};
    } catch (...) {
        return {CudaKernelAdapterCode::resource_exhausted, 0, true};
    }
}

CudaKernelAdapterResult CudaSyntheticFp32Adapter::launch(
    const CudaJobDescriptor& job,
    cudaStream_t stream) noexcept {
    try {
        if (!initialized_ || device_input_ == nullptr ||
            device_output_ == nullptr) {
            return {
                CudaKernelAdapterCode::not_initialized,
                0,
                false,
                "Synthetic CUDA adapter is not initialized",
            };
        }
        if (job.kernel_id != kCudaSyntheticExecutionKernelId ||
            stream == nullptr || stream != stream_) {
            return {
                CudaKernelAdapterCode::invalid_context,
                0,
                false,
                "Synthetic CUDA launch did not use its exact kernel and stream",
            };
        }
        if (launch_pending_validation_) {
            return {
                CudaKernelAdapterCode::launch_failed,
                0,
                true,
                "Synthetic CUDA adapter has an unvalidated prior launch",
            };
        }

        std::fill(std::begin(host_output_), std::end(host_output_), -99.0F);
        cudaError_t status = cudaMemcpyAsync(
            device_input_,
            host_input_,
            sizeof(host_input_),
            cudaMemcpyHostToDevice,
            stream_);
        if (status != cudaSuccess) {
            return cuda_failure(
                CudaKernelAdapterCode::launch_failed,
                status,
                "cudaMemcpyAsync synthetic input",
                true);
        }

        status = prometheus::cuda::kernels::launch_activation_f32(
            device_input_,
            device_output_,
            kCudaSyntheticProbeElementCount,
            prometheus::cuda::kernels::ActivationType::Relu,
            0.0F,
            stream_);
        if (status != cudaSuccess) {
            return cuda_failure(
                CudaKernelAdapterCode::launch_failed,
                status,
                "launch_activation_f32",
                true);
        }

        status = cudaMemcpyAsync(
            host_output_,
            device_output_,
            sizeof(host_output_),
            cudaMemcpyDeviceToHost,
            stream_);
        if (status != cudaSuccess) {
            return cuda_failure(
                CudaKernelAdapterCode::launch_failed,
                status,
                "cudaMemcpyAsync synthetic output",
                true);
        }

        launch_pending_validation_ = true;
        return {};
    } catch (...) {
        return {CudaKernelAdapterCode::resource_exhausted, 0, true};
    }
}

CudaKernelAdapterResult
CudaSyntheticFp32Adapter::validate_completion() noexcept {
    try {
        if (!initialized_ || !launch_pending_validation_) {
            return {
                CudaKernelAdapterCode::not_initialized,
                0,
                false,
                "Synthetic CUDA adapter has no completed launch to validate",
            };
        }

        for (std::size_t index = 0;
             index < kCudaSyntheticProbeElementCount;
             ++index) {
            const float expected = std::max(host_input_[index], 0.0F);
            if (host_output_[index] != expected) {
                launch_pending_validation_ = false;
                return {
                    CudaKernelAdapterCode::completion_validation_failed,
                    0,
                    false,
                    "Synthetic CUDA FP32 output validation failed",
                };
            }
        }
        launch_pending_validation_ = false;
        return {};
    } catch (...) {
        launch_pending_validation_ = false;
        return {CudaKernelAdapterCode::resource_exhausted, 0, false};
    }
}

CudaKernelAdapterResult CudaSyntheticFp32Adapter::shutdown() noexcept {
    CudaKernelAdapterResult first_failure{};
    try {
        if (device_output_ != nullptr) {
            const cudaError_t status = cudaFree(device_output_);
            device_output_ = nullptr;
            if (status != cudaSuccess) {
                first_failure = cuda_failure(
                    CudaKernelAdapterCode::shutdown_failed,
                    status,
                    "cudaFree synthetic output",
                    true);
            }
        }
        if (device_input_ != nullptr) {
            const cudaError_t status = cudaFree(device_input_);
            device_input_ = nullptr;
            if (status != cudaSuccess && first_failure.ok()) {
                first_failure = cuda_failure(
                    CudaKernelAdapterCode::shutdown_failed,
                    status,
                    "cudaFree synthetic input",
                    true);
            }
        }
        runtime_ordinal_snapshot_ = -1;
        stream_ = nullptr;
        initialized_ = false;
        launch_pending_validation_ = false;
        return first_failure;
    } catch (...) {
        runtime_ordinal_snapshot_ = -1;
        stream_ = nullptr;
        initialized_ = false;
        launch_pending_validation_ = false;
        return {CudaKernelAdapterCode::resource_exhausted, 0, true};
    }
}

CudaKernelAdapterResult CudaSyntheticFp32Adapter::cuda_failure(
    CudaKernelAdapterCode code,
    cudaError_t error,
    const char* operation,
    bool worker_fatal) const noexcept {
    try {
        return {
            code,
            static_cast<int>(error),
            worker_fatal,
            cuda_error_message(error, operation),
        };
    } catch (...) {
        return {code, static_cast<int>(error), worker_fatal};
    }
}

CudaKernelDescriptor make_cuda_synthetic_execution_descriptor() noexcept {
    try {
        CudaKernelDescriptor descriptor{};
        descriptor.kernel_id = kCudaSyntheticExecutionKernelId;
        descriptor.display_name = "Bounded synthetic FP32 execution probe";
        descriptor.family = CudaKernelFamily::activation;
        descriptor.scalar_types = {CudaScalarType::fp32};
        descriptor.minimum_compute_capability = {6, 1};
        descriptor.cuda_12_release_support = CudaSupportState::supported;
        descriptor.cuda_13_release_support = CudaSupportState::supported;
        descriptor.binary_state = CudaKernelBinaryState::available;
        descriptor.binding_state = CudaKernelBindingState::adapter_available;
        descriptor.source_file =
            "src/backends/cuda/cuda_kernel_adapter.cpp";
        descriptor.adapter_name = "cuda_synthetic_relu_fp32";
        return descriptor;
    } catch (...) {
        return {};
    }
}

const char* to_string(CudaKernelAdapterCode value) noexcept {
    switch (value) {
        case CudaKernelAdapterCode::success:
            return "success";
        case CudaKernelAdapterCode::invalid_adapter:
            return "invalid_adapter";
        case CudaKernelAdapterCode::invalid_kernel_id:
            return "invalid_kernel_id";
        case CudaKernelAdapterCode::duplicate_kernel_id:
            return "duplicate_kernel_id";
        case CudaKernelAdapterCode::capacity_reached:
            return "capacity_reached";
        case CudaKernelAdapterCode::registry_active:
            return "registry_active";
        case CudaKernelAdapterCode::not_found:
            return "not_found";
        case CudaKernelAdapterCode::invalid_context:
            return "invalid_context";
        case CudaKernelAdapterCode::not_initialized:
            return "not_initialized";
        case CudaKernelAdapterCode::already_initialized:
            return "already_initialized";
        case CudaKernelAdapterCode::initialization_failed:
            return "initialization_failed";
        case CudaKernelAdapterCode::launch_failed:
            return "launch_failed";
        case CudaKernelAdapterCode::completion_validation_failed:
            return "completion_validation_failed";
        case CudaKernelAdapterCode::shutdown_failed:
            return "shutdown_failed";
        case CudaKernelAdapterCode::resource_exhausted:
            return "resource_exhausted";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cuda
