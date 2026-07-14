#include "kernels/learning_kernels.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

void check_cuda(
    const cudaError_t error,
    const char* expression,
    const char* file,
    const int line)
{
    if (error == cudaSuccess) {
        return;
    }

    std::cerr
        << "CUDA failure at "
        << file << ':'
        << line << " for "
        << expression << ": "
        << cudaGetErrorString(error)
        << '\n';

    std::exit(EXIT_FAILURE);
}

#define CHECK_CUDA(expression) \
    check_cuda((expression), #expression, __FILE__, __LINE__)

void require(
    const bool condition,
    const std::string& message)
{
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename T>
bool nearly_equal(
    const T actual,
    const T expected,
    const T tolerance)
{
    const T scale =
        std::max({
            static_cast<T>(1),
            std::abs(actual),
            std::abs(expected),
        });

    return std::abs(actual - expected) <= tolerance * scale;
}

template <typename T>
void compare_vectors(
    const std::vector<T>& actual,
    const std::vector<T>& expected,
    const T tolerance,
    const std::string& label)
{
    require(
        actual.size() == expected.size(),
        label + " size mismatch");

    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(
            nearly_equal(
                actual[index],
                expected[index],
                tolerance),
            label + " mismatch at index " +
                std::to_string(index));
    }
}

template <typename T>
T clamp_symmetric(
    const T value,
    const T limit)
{
    if (limit <= static_cast<T>(0)) {
        return value;
    }

    return std::max(-limit, std::min(value, limit));
}

template <typename T>
struct ReferenceLearning {
    std::vector<T> final_weights;
    std::vector<T> updates;
    std::vector<T> row_l1;
    std::vector<T> row_l2;
    std::vector<T> row_max;
};

template <typename T>
ReferenceLearning<T> reference_learning(
    const std::vector<T>& presynaptic,
    const std::vector<T>& teaching,
    const std::vector<T>& initial_weights,
    const prometheus::cuda::kernels::LearningShape shape,
    const prometheus::cuda::kernels::LearningRule rule,
    const prometheus::cuda::kernels::LearningBatchReduction reduction,
    const std::vector<std::uint8_t>* row_mask,
    const std::vector<std::uint8_t>* weight_mask,
    const T learning_rate,
    const T weight_decay,
    const T update_clip,
    const T weight_clip,
    const bool apply_update)
{
    using namespace prometheus::cuda::kernels;

    ReferenceLearning<T> result;
    result.final_weights = initial_weights;
    result.updates.assign(initial_weights.size(), static_cast<T>(0));
    result.row_l1.assign(shape.output_dim, static_cast<T>(0));
    result.row_l2.assign(shape.output_dim, static_cast<T>(0));
    result.row_max.assign(shape.output_dim, static_cast<T>(0));

    for (std::size_t row = 0; row < shape.output_dim; ++row) {
        const bool row_enabled =
            row_mask == nullptr || (*row_mask)[row] != 0U;

        T row_l2_squared = static_cast<T>(0);

        for (
            std::size_t column = 0;
            column < shape.input_dim;
            ++column)
        {
            const std::size_t weight_index =
                row * shape.input_dim + column;

            const bool weight_enabled =
                weight_mask == nullptr ||
                (*weight_mask)[weight_index] != 0U;

            T effective_delta = static_cast<T>(0);

            if (row_enabled && weight_enabled) {
                const T current_weight =
                    initial_weights[weight_index];

                T correlation = static_cast<T>(0);

                for (
                    std::size_t batch = 0;
                    batch < shape.batch_count;
                    ++batch)
                {
                    const T pre =
                        presynaptic[
                            batch * shape.input_dim + column
                        ];

                    const T signal =
                        teaching[
                            batch * shape.output_dim + row
                        ];

                    if (rule == LearningRule::Oja) {
                        correlation +=
                            signal * pre -
                            signal * signal * current_weight;
                    } else {
                        correlation += signal * pre;
                    }
                }

                if (
                    reduction ==
                    LearningBatchReduction::Mean)
                {
                    correlation /=
                        static_cast<T>(shape.batch_count);
                }

                T proposed_delta =
                    learning_rate *
                    (
                        correlation -
                        weight_decay * current_weight
                    );

                proposed_delta =
                    clamp_symmetric(
                        proposed_delta,
                        update_clip);

                const T proposed_weight =
                    clamp_symmetric(
                        current_weight + proposed_delta,
                        weight_clip);

                effective_delta =
                    proposed_weight - current_weight;

                if (apply_update) {
                    result.final_weights[weight_index] =
                        proposed_weight;
                }
            }

            result.updates[weight_index] =
                effective_delta;

            const T absolute_delta =
                std::abs(effective_delta);

            result.row_l1[row] += absolute_delta;
            row_l2_squared +=
                effective_delta * effective_delta;
            result.row_max[row] =
                std::max(
                    result.row_max[row],
                    absolute_delta);
        }

        result.row_l2[row] =
            std::sqrt(row_l2_squared);
    }

    return result;
}

template <typename T>
T* allocate_and_copy(const std::vector<T>& values)
{
    T* device = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device),
            values.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMemcpy(
            device,
            values.data(),
            values.size() * sizeof(T),
            cudaMemcpyHostToDevice));

    return device;
}

template <typename T>
std::vector<T> copy_back(
    const T* device,
    const std::size_t count)
{
    std::vector<T> values(count);

    CHECK_CUDA(
        cudaMemcpy(
            values.data(),
            device,
            count * sizeof(T),
            cudaMemcpyDeviceToHost));

    return values;
}

template <typename T>
void launch_learning(
    const T* device_presynaptic,
    const T* device_teaching,
    T* device_weights,
    T* device_updates,
    T* device_l1,
    T* device_l2,
    T* device_max,
    const prometheus::cuda::kernels::LearningShape shape,
    const prometheus::cuda::kernels::LearningRule rule,
    const prometheus::cuda::kernels::LearningBatchReduction reduction,
    const std::uint8_t* device_row_mask,
    const std::uint8_t* device_weight_mask,
    const T learning_rate,
    const T weight_decay,
    const T update_clip,
    const T weight_clip,
    const bool apply_update)
{
    using namespace prometheus::cuda::kernels;

    if constexpr (std::is_same_v<T, float>) {
        CHECK_CUDA(
            launch_local_learning_f32(
                device_presynaptic,
                device_teaching,
                device_weights,
                device_updates,
                device_l1,
                device_l2,
                device_max,
                shape,
                rule,
                reduction,
                device_row_mask,
                device_weight_mask,
                LearningParametersF32{
                    learning_rate,
                    weight_decay,
                    update_clip,
                    weight_clip,
                },
                apply_update));
    } else {
        CHECK_CUDA(
            launch_local_learning_f64(
                device_presynaptic,
                device_teaching,
                device_weights,
                device_updates,
                device_l1,
                device_l2,
                device_max,
                shape,
                rule,
                reduction,
                device_row_mask,
                device_weight_mask,
                LearningParametersF64{
                    learning_rate,
                    weight_decay,
                    update_clip,
                    weight_clip,
                },
                apply_update));
    }
}

template <typename T>
void run_shadow_rule_case(
    const prometheus::cuda::kernels::LearningRule rule,
    const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    const LearningShape shape{
        2U,
        3U,
        2U,
    };

    const std::vector<T> presynaptic{
        static_cast<T>(1),
        static_cast<T>(2),
        static_cast<T>(-1),

        static_cast<T>(0.5),
        static_cast<T>(-2),
        static_cast<T>(3),
    };

    const std::vector<T> teaching{
        static_cast<T>(0.25),
        static_cast<T>(-0.5),

        static_cast<T>(1.5),
        static_cast<T>(0.75),
    };

    const std::vector<T> weights{
        static_cast<T>(0.1),
        static_cast<T>(-0.2),
        static_cast<T>(0.3),

        static_cast<T>(-0.4),
        static_cast<T>(0.5),
        static_cast<T>(-0.6),
    };

    const T learning_rate = static_cast<T>(0.05);
    const T weight_decay = static_cast<T>(0.01);
    const T update_clip = static_cast<T>(0.08);
    const T weight_clip = static_cast<T>(0.55);

    const auto expected =
        reference_learning(
            presynaptic,
            teaching,
            weights,
            shape,
            rule,
            LearningBatchReduction::Mean,
            nullptr,
            nullptr,
            learning_rate,
            weight_decay,
            update_clip,
            weight_clip,
            false);

    T* device_presynaptic =
        allocate_and_copy(presynaptic);

    T* device_teaching =
        allocate_and_copy(teaching);

    T* device_weights =
        allocate_and_copy(weights);

    T* device_updates = nullptr;
    T* device_l1 = nullptr;
    T* device_l2 = nullptr;
    T* device_max = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_updates),
            weights.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_l1),
            shape.output_dim * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_l2),
            shape.output_dim * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_max),
            shape.output_dim * sizeof(T)));

    launch_learning(
        device_presynaptic,
        device_teaching,
        device_weights,
        device_updates,
        device_l1,
        device_l2,
        device_max,
        shape,
        rule,
        LearningBatchReduction::Mean,
        nullptr,
        nullptr,
        learning_rate,
        weight_decay,
        update_clip,
        weight_clip,
        false);

    compare_vectors(
        copy_back(device_weights, weights.size()),
        weights,
        tolerance,
        "shadow weights");

    compare_vectors(
        copy_back(device_updates, weights.size()),
        expected.updates,
        tolerance,
        "shadow updates");

    compare_vectors(
        copy_back(device_l1, shape.output_dim),
        expected.row_l1,
        tolerance,
        "row L1");

    compare_vectors(
        copy_back(device_l2, shape.output_dim),
        expected.row_l2,
        tolerance,
        "row L2");

    compare_vectors(
        copy_back(device_max, shape.output_dim),
        expected.row_max,
        tolerance,
        "row max");

    CHECK_CUDA(cudaFree(device_max));
    CHECK_CUDA(cudaFree(device_l2));
    CHECK_CUDA(cudaFree(device_l1));
    CHECK_CUDA(cudaFree(device_updates));
    CHECK_CUDA(cudaFree(device_weights));
    CHECK_CUDA(cudaFree(device_teaching));
    CHECK_CUDA(cudaFree(device_presynaptic));
}

template <typename T>
void run_masked_apply_case(const T tolerance)
{
    using namespace prometheus::cuda::kernels;

    const LearningShape shape{
        2U,
        3U,
        2U,
    };

    const std::vector<T> presynaptic{
        static_cast<T>(1),
        static_cast<T>(-1),
        static_cast<T>(2),

        static_cast<T>(3),
        static_cast<T>(0.5),
        static_cast<T>(-2),
    };

    const std::vector<T> error{
        static_cast<T>(0.5),
        static_cast<T>(-1),

        static_cast<T>(1.25),
        static_cast<T>(2),
    };

    const std::vector<T> weights{
        static_cast<T>(0.2),
        static_cast<T>(-0.3),
        static_cast<T>(0.4),

        static_cast<T>(-0.5),
        static_cast<T>(0.6),
        static_cast<T>(-0.7),
    };

    const std::vector<std::uint8_t> row_mask{
        1U,
        0U,
    };

    const std::vector<std::uint8_t> weight_mask{
        1U,
        0U,
        1U,

        1U,
        1U,
        1U,
    };

    const T learning_rate = static_cast<T>(0.1);
    const T weight_decay = static_cast<T>(0.02);
    const T update_clip = static_cast<T>(0.05);
    const T weight_clip = static_cast<T>(0.45);

    const auto expected =
        reference_learning(
            presynaptic,
            error,
            weights,
            shape,
            LearningRule::PredictiveDelta,
            LearningBatchReduction::Sum,
            &row_mask,
            &weight_mask,
            learning_rate,
            weight_decay,
            update_clip,
            weight_clip,
            true);

    T* device_presynaptic =
        allocate_and_copy(presynaptic);

    T* device_error =
        allocate_and_copy(error);

    T* device_weights =
        allocate_and_copy(weights);

    std::uint8_t* device_row_mask = nullptr;
    std::uint8_t* device_weight_mask = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_row_mask),
            row_mask.size() * sizeof(std::uint8_t)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_weight_mask),
            weight_mask.size() * sizeof(std::uint8_t)));

    CHECK_CUDA(
        cudaMemcpy(
            device_row_mask,
            row_mask.data(),
            row_mask.size() * sizeof(std::uint8_t),
            cudaMemcpyHostToDevice));

    CHECK_CUDA(
        cudaMemcpy(
            device_weight_mask,
            weight_mask.data(),
            weight_mask.size() * sizeof(std::uint8_t),
            cudaMemcpyHostToDevice));

    T* device_updates = nullptr;
    T* device_l1 = nullptr;
    T* device_l2 = nullptr;
    T* device_max = nullptr;

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_updates),
            weights.size() * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_l1),
            shape.output_dim * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_l2),
            shape.output_dim * sizeof(T)));

    CHECK_CUDA(
        cudaMalloc(
            reinterpret_cast<void**>(&device_max),
            shape.output_dim * sizeof(T)));

    launch_learning(
        device_presynaptic,
        device_error,
        device_weights,
        device_updates,
        device_l1,
        device_l2,
        device_max,
        shape,
        LearningRule::PredictiveDelta,
        LearningBatchReduction::Sum,
        device_row_mask,
        device_weight_mask,
        learning_rate,
        weight_decay,
        update_clip,
        weight_clip,
        true);

    compare_vectors(
        copy_back(device_weights, weights.size()),
        expected.final_weights,
        tolerance,
        "applied weights");

    compare_vectors(
        copy_back(device_updates, weights.size()),
        expected.updates,
        tolerance,
        "applied updates");

    compare_vectors(
        copy_back(device_l1, shape.output_dim),
        expected.row_l1,
        tolerance,
        "masked row L1");

    compare_vectors(
        copy_back(device_l2, shape.output_dim),
        expected.row_l2,
        tolerance,
        "masked row L2");

    compare_vectors(
        copy_back(device_max, shape.output_dim),
        expected.row_max,
        tolerance,
        "masked row max");

    CHECK_CUDA(cudaFree(device_max));
    CHECK_CUDA(cudaFree(device_l2));
    CHECK_CUDA(cudaFree(device_l1));
    CHECK_CUDA(cudaFree(device_updates));
    CHECK_CUDA(cudaFree(device_weight_mask));
    CHECK_CUDA(cudaFree(device_row_mask));
    CHECK_CUDA(cudaFree(device_weights));
    CHECK_CUDA(cudaFree(device_error));
    CHECK_CUDA(cudaFree(device_presynaptic));
}

}  // namespace

int main()
{
    using prometheus::cuda::kernels::LearningRule;

    const LearningRule rules[]{
        LearningRule::Hebbian,
        LearningRule::Oja,
        LearningRule::PredictiveDelta,
    };

    for (const LearningRule rule : rules) {
        run_shadow_rule_case<float>(
            rule,
            3.0e-5F);

        run_shadow_rule_case<double>(
            rule,
            1.0e-10);
    }

    run_masked_apply_case<float>(3.0e-5F);
    run_masked_apply_case<double>(1.0e-10);

    std::cout
        << "Prometheus local learning CUDA kernels passed\n";

    return EXIT_SUCCESS;
}
