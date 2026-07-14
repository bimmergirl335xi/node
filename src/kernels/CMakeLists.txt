add_library(
    prometheus_cuda_kernels
    STATIC
        activation_kernels.cu
        reduction_kernels.cu
        dense_kernels.cu
        normalization_kernels.cu
        temporal_kernels.cu
        learning_kernels.cu
        optimizer_kernels.cu
)

add_library(
    prometheus::cuda_kernels
    ALIAS
    prometheus_cuda_kernels
)

target_include_directories(
    prometheus_cuda_kernels
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    prometheus_cuda_kernels
    PUBLIC
        CUDA::cudart
)

set_target_properties(
    prometheus_cuda_kernels
    PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        POSITION_INDEPENDENT_CODE ON
)

target_compile_options(
    prometheus_cuda_kernels
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>
        $<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:Debug>>:-G;-lineinfo>
        $<$<AND:$<COMPILE_LANGUAGE:CUDA>,$<CONFIG:RelWithDebInfo>>:-lineinfo>
)

if(PROMETHEUS_CUDA_FAST_MATH)
    target_compile_options(
        prometheus_cuda_kernels
        PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>
    )
endif()
