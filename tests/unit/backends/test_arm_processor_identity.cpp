#include "arm_processor_identity.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace arm = prometheus::backends::arm;

namespace {

[[nodiscard]] bool expect(
    bool condition,
    const std::string& message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        return false;
    }
    return true;
}

[[nodiscard]] bool has_issue(
    const arm::ArmProcessorIdentityQueryResult& result,
    arm::ArmProcessorIdentityIssueCode code) {
    for (const arm::ArmProcessorIdentityIssue& issue :
         result.snapshot.issues) {
        if (issue.code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool test_homogeneous_grouping() {
    const std::string cpuinfo = R"(
processor : 0
CPU implementer : 0x41
CPU architecture : 8
CPU variant : 0x0
CPU part : 0xd0b
CPU revision : 1

processor : 1
CPU implementer : 0x41
CPU architecture : 8
CPU variant : 0x0
CPU part : 0xd0b
CPU revision : 1
)";

    const auto result = arm::parse_arm_proc_cpuinfo(cpuinfo, {0, 1});
    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::success,
        "homogeneous parse should succeed");

    passed &= expect(
        result.snapshot.identities.size() == 1,
        "identical processors should form one identity group");

    passed &= expect(
        !result.snapshot.heterogeneous,
        "one identity group should not be heterogeneous");

    passed &= expect(
        result.snapshot.identities[0].logical_processor_ids.size() == 2,
        "identity group should retain both processors");

    passed &= expect(
        result.snapshot.identities[0].implementer_text == "Arm",
        "implementer text should decode");

    passed &= expect(
        result.snapshot.identities[0].part_text == "Cortex-A76",
        "part text should decode");

    return passed;
}

[[nodiscard]] bool test_heterogeneous_grouping() {
    const std::string cpuinfo = R"(
processor : 0
CPU implementer : 0x41
CPU architecture : 8
CPU variant : 0x0
CPU part : 0xd05
CPU revision : 0

processor : 1
CPU implementer : 0x41
CPU architecture : 8
CPU variant : 0x0
CPU part : 0xd05
CPU revision : 0

processor : 2
CPU implementer : 0x41
CPU architecture : 8
CPU variant : 0x0
CPU part : 0xd0b
CPU revision : 1
)";

    const auto result = arm::parse_arm_proc_cpuinfo(
        cpuinfo,
        {0, 1, 2});

    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::success,
        "heterogeneous parse should still succeed");

    passed &= expect(
        result.snapshot.identities.size() == 2,
        "different parts should form two groups");

    passed &= expect(
        result.snapshot.heterogeneous,
        "two identity groups should be heterogeneous");

    return passed;
}

[[nodiscard]] bool test_missing_target_is_explicit() {
    const std::string cpuinfo = R"(
processor : 0
CPU implementer : 0x41
CPU architecture : 8
CPU part : 0xd0c
)";

    const auto result = arm::parse_arm_proc_cpuinfo(cpuinfo, {0, 1});
    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::partial_success,
        "missing target should be partial success");

    passed &= expect(
        result.snapshot.missing_target_logical_processor_ids.size() == 1 &&
            result.snapshot.missing_target_logical_processor_ids[0] == 1,
        "missing target ID should be retained");

    return passed;
}

[[nodiscard]] bool test_malformed_part_remains_unobserved() {
    const std::string cpuinfo = R"(
processor : 4
CPU implementer : 0x41
CPU architecture : 8
CPU part : definitely-not-a-part
)";

    const auto result = arm::parse_arm_proc_cpuinfo(cpuinfo, {4});
    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::partial_success,
        "malformed part should be partial success");

    passed &= expect(
        result.snapshot.identities.size() == 1,
        "record should still be retained");

    passed &= expect(
        !result.snapshot.identities[0].part_observed,
        "malformed part must remain unobserved");

    passed &= expect(
        !result.snapshot.identities[0].signature_complete(),
        "incomplete signature must remain explicit");

    return passed;
}

[[nodiscard]] bool test_non_target_processors_are_ignored() {
    const std::string cpuinfo = R"(
processor : 0
CPU implementer : 0x41
CPU architecture : 8
CPU part : 0xd05

processor : 1
CPU implementer : 0x41
CPU architecture : 8
CPU part : 0xd49
)";

    const auto result = arm::parse_arm_proc_cpuinfo(cpuinfo, {1});
    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::success,
        "single selected target should succeed");

    passed &= expect(
        result.snapshot.identities.size() == 1,
        "only selected target identity should be retained");

    passed &= expect(
        result.snapshot.identities[0].part == 0xd49,
        "selected target part should be retained");

    passed &= expect(
        result.snapshot.identities[0].logical_processor_ids.size() == 1 &&
            result.snapshot.identities[0].logical_processor_ids[0] == 1,
        "non-target processor should not join the group");

    return passed;
}

[[nodiscard]] bool test_empty_target_fails() {
    const auto result = arm::parse_arm_proc_cpuinfo("", {});

    return expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::query_failed,
        "empty target set should fail");
}

[[nodiscard]] bool test_duplicate_and_malformed_records_are_explicit() {
    const std::string cpuinfo = R"(
processor : invalid
CPU implementer : 0x41

processor : 0
CPU implementer : 0x41
CPU architecture : malformed
CPU variant : malformed
CPU part : 0xd05
CPU revision : malformed

processor : 0
CPU implementer : 0x41
CPU architecture : 8
CPU part : 0xd0b

processor : 1
CPU implementer : 0x41
)";

    const auto result = arm::parse_arm_proc_cpuinfo(cpuinfo, {0, 1});
    bool passed = true;

    passed &= expect(
        result.status.code ==
            arm::ArmProcessorIdentityQueryCode::partial_success,
        "duplicate and malformed records should be partial success");
    passed &= expect(
        has_issue(
            result,
            arm::ArmProcessorIdentityIssueCode::malformed_processor_id),
        "malformed processor IDs must be explicit");
    passed &= expect(
        has_issue(
            result,
            arm::ArmProcessorIdentityIssueCode::duplicate_processor_record),
        "duplicate processor records must be explicit");
    passed &= expect(
        has_issue(
            result,
            arm::ArmProcessorIdentityIssueCode::malformed_architecture) &&
            has_issue(
                result,
                arm::ArmProcessorIdentityIssueCode::malformed_variant) &&
            has_issue(
                result,
                arm::ArmProcessorIdentityIssueCode::malformed_revision),
        "malformed identity fields must remain explicit");
    passed &= expect(
        has_issue(
            result,
            arm::ArmProcessorIdentityIssueCode::incomplete_identity),
        "incomplete identity records must remain explicit");
    passed &= expect(
        !result.snapshot.heterogeneous,
        "incomplete signatures must not prove heterogeneity");

    return passed;
}

}  // namespace

int main() {
    bool passed = true;

    passed &= test_homogeneous_grouping();
    passed &= test_heterogeneous_grouping();
    passed &= test_missing_target_is_explicit();
    passed &= test_malformed_part_remains_unobserved();
    passed &= test_non_target_processors_are_ignored();
    passed &= test_empty_target_fails();
    passed &= test_duplicate_and_malformed_records_are_explicit();

    if (!passed) {
        return EXIT_FAILURE;
    }

    std::cout << "ARM processor identity tests passed\n";
    return EXIT_SUCCESS;
}
