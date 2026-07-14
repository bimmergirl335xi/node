#ifndef SRC_BACKENDS_ARM_ARM_PROCESSOR_IDENTITY_HPP
#define SRC_BACKENDS_ARM_ARM_PROCESSOR_IDENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "arm_capabilities.hpp"

namespace prometheus::backends::arm {

enum class ArmProcessorIdentitySource : std::uint8_t {
    none = 0,
    injected_proc_cpuinfo,
    proc_cpuinfo,
};

enum class ArmProcessorIdentityIssueCode : std::uint8_t {
    empty_target_set = 0,
    cpuinfo_unavailable,
    malformed_processor_id,
    duplicate_processor_record,
    target_processor_missing,
    malformed_implementer,
    malformed_architecture,
    malformed_variant,
    malformed_part,
    malformed_revision,
    incomplete_identity,
};

struct ArmProcessorIdentityIssue {
    ArmProcessorIdentityIssueCode code =
        ArmProcessorIdentityIssueCode::cpuinfo_unavailable;
    int os_cpu_id = -1;
    bool fatal = false;
    std::string message{};
};

enum class ArmProcessorIdentityQueryCode : std::uint8_t {
    success = 0,
    partial_success,
    query_failed,
};

struct ArmProcessorIdentityQueryStatus {
    ArmProcessorIdentityQueryCode code =
        ArmProcessorIdentityQueryCode::success;
    std::string message{};

    [[nodiscard]] bool completed() const noexcept {
        return code == ArmProcessorIdentityQueryCode::success ||
               code == ArmProcessorIdentityQueryCode::partial_success;
    }
};

struct ArmProcessorIdentitySnapshot {
    ArmProcessorIdentitySource source = ArmProcessorIdentitySource::none;
    std::vector<ArmProcessorIdentity> identities{};
    std::vector<int> target_logical_processor_ids{};
    std::vector<int> observed_target_logical_processor_ids{};
    std::vector<int> missing_target_logical_processor_ids{};
    bool heterogeneous = false;
    std::vector<ArmProcessorIdentityIssue> issues{};
};

struct ArmProcessorIdentityQueryResult {
    ArmProcessorIdentityQueryStatus status{};
    ArmProcessorIdentitySnapshot snapshot{};
};

// Parses injected /proc/cpuinfo text so ARM identity grouping remains fully
// testable on non-ARM validation hosts. Only requested target processors are
// retained. Processor signatures are observational metadata, not dispatch
// authority and not durable backend identities.
[[nodiscard]] ArmProcessorIdentityQueryResult parse_arm_proc_cpuinfo(
    std::string_view cpuinfo_text,
    const std::vector<int>& target_logical_processor_ids);

// Read-only Linux provider. It does not alter affinity, topology, worker state,
// scheduling, or any attached accelerator.
[[nodiscard]] ArmProcessorIdentityQueryResult
query_linux_arm_processor_identities(
    const std::vector<int>& target_logical_processor_ids,
    const std::string& cpuinfo_path = "/proc/cpuinfo");

[[nodiscard]] std::string arm_implementer_name(std::uint32_t implementer);
[[nodiscard]] std::string arm_part_name(
    std::uint32_t implementer,
    std::uint32_t part);

[[nodiscard]] const char* to_string(
    ArmProcessorIdentitySource value) noexcept;
[[nodiscard]] const char* to_string(
    ArmProcessorIdentityIssueCode value) noexcept;
[[nodiscard]] const char* to_string(
    ArmProcessorIdentityQueryCode value) noexcept;

}  // namespace prometheus::backends::arm

#endif  // SRC_BACKENDS_ARM_ARM_PROCESSOR_IDENTITY_HPP
