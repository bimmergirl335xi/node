#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

#include "arm_capabilities.hpp"
#include "arm_processor_identity.hpp"
#include "cpu_capabilities.hpp"
#include "cpu_topology.hpp"

namespace arm = prometheus::backends::arm;
namespace cpu = prometheus::backends::cpu;

namespace {

void print_processor_ids(const std::vector<int>& ids) {
    if (ids.empty()) {
        std::cout << "none";
        return;
    }

    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index != 0) {
            std::cout << ',';
        }
        std::cout << ids[index];
    }
}

void print_coverage(
    std::string_view name,
    const cpu::CpuFeatureCoverage& coverage) {
    std::cout << "  " << std::left << std::setw(20) << name
              << " common=" << cpu::to_string(coverage.common)
              << " any=" << cpu::to_string(coverage.any)
              << " observed=" << coverage.observed_processor_count
              << '/' << coverage.target_processor_count
              << " supported=" << coverage.supported_processor_count
              << '\n';
}

void print_arm_capability_issues(
    const std::vector<arm::ArmCapabilityIssue>& issues) {
    if (issues.empty()) {
        std::cout << "issues: none\n";
        return;
    }

    std::cout << "issues:\n";
    for (const arm::ArmCapabilityIssue& issue : issues) {
        std::cout << "  - code=" << arm::to_string(issue.code)
                  << " fatal=" << (issue.fatal ? "yes" : "no")
                  << " cpu=" << issue.os_cpu_id
                  << " message=" << issue.message
                  << '\n';
    }
}

void print_identity_issues(
    const std::vector<arm::ArmProcessorIdentityIssue>& issues) {
    if (issues.empty()) {
        std::cout << "identity_issues: none\n";
        return;
    }

    std::cout << "identity_issues:\n";
    for (const arm::ArmProcessorIdentityIssue& issue : issues) {
        std::cout << "  - code=" << arm::to_string(issue.code)
                  << " fatal=" << (issue.fatal ? "yes" : "no")
                  << " cpu=" << issue.os_cpu_id
                  << " message=" << issue.message
                  << '\n';
    }
}

void print_identity_groups(
    const arm::ArmProcessorIdentityQueryResult& identity_result) {
    std::cout << "identity_status: "
              << arm::to_string(identity_result.status.code)
              << '\n';
    std::cout << "identity_source: "
              << arm::to_string(identity_result.snapshot.source)
              << '\n';
    std::cout << "identity_heterogeneous: "
              << (identity_result.snapshot.heterogeneous ? "yes" : "no")
              << '\n';
    std::cout << "identity_groups: "
              << identity_result.snapshot.identities.size()
              << '\n';

    for (std::size_t index = 0;
         index < identity_result.snapshot.identities.size();
         ++index) {
        const arm::ArmProcessorIdentity& identity =
            identity_result.snapshot.identities[index];

        std::cout << "  group[" << index << "]\n";
        std::cout << "    implementer: "
                  << (identity.implementer_observed
                          ? identity.implementer_text
                          : "unknown")
                  << '\n';
        std::cout << "    architecture: ";
        if (identity.architecture_observed) {
            std::cout << identity.architecture;
        } else {
            std::cout << "unknown";
        }
        std::cout << '\n';
        std::cout << "    variant: ";
        if (identity.variant_observed) {
            std::cout << identity.variant;
        } else {
            std::cout << "unknown";
        }
        std::cout << '\n';
        std::cout << "    part: "
                  << (identity.part_observed
                          ? identity.part_text
                          : "unknown")
                  << '\n';
        std::cout << "    revision: ";
        if (identity.revision_observed) {
            std::cout << identity.revision;
        } else {
            std::cout << "unknown";
        }
        std::cout << '\n';
        std::cout << "    logical_processors: ";
        print_processor_ids(identity.logical_processor_ids);
        std::cout << '\n';
    }

    std::cout << "missing_target_processors: ";
    print_processor_ids(
        identity_result.snapshot.missing_target_logical_processor_ids);
    std::cout << '\n';

    print_identity_issues(identity_result.snapshot.issues);
}

}  // namespace

int main() {
    const cpu::CpuTopologyQueryResult topology_result =
        cpu::query_cpu_topology();

    if (!topology_result.status.completed()) {
        std::cerr << "CPU topology query failed: "
                  << cpu::to_string(topology_result.status.code)
                  << " - " << topology_result.status.message
                  << '\n';
        return EXIT_FAILURE;
    }

    const cpu::CpuCapabilityQueryResult cpu_result =
        cpu::query_cpu_capabilities(topology_result.topology);

    if (!cpu_result.status.completed()) {
        std::cerr << "CPU capability query failed: "
                  << cpu::to_string(cpu_result.status.code)
                  << " - " << cpu_result.status.message
                  << '\n';
        return EXIT_FAILURE;
    }

    const arm::ArmCapabilityQueryResult arm_result =
        arm::query_arm_capabilities(cpu_result.capabilities);
    const arm::ArmCapabilitySnapshot& snapshot = arm_result.capabilities;
    const arm::ArmIsaCapabilities& isa = snapshot.isa;

    std::cout << "Prometheus ARM capability probe\n";
    std::cout << "query_status: " << arm::to_string(arm_result.status.code)
              << '\n';
    std::cout << "query_message: " << arm_result.status.message << '\n';
    std::cout << "execution_state: " << arm::to_string(isa.execution_state)
              << '\n';
    std::cout << "capability_source: " << arm::to_string(isa.source)
              << '\n';
    std::cout << "common_profile: " << arm::to_string(isa.common_profile)
              << '\n';
    std::cout << "feature_sets_heterogeneous: "
              << (isa.feature_sets_heterogeneous ? "yes" : "no")
              << '\n';
    std::cout << "target_logical_processors: ";
    print_processor_ids(snapshot.target_logical_processor_ids);
    std::cout << '\n';

    if (arm_result.status.code ==
        arm::ArmCapabilityQueryCode::unsupported_architecture) {
        print_arm_capability_issues(snapshot.issues);
        return EXIT_SUCCESS;
    }

    if (!arm_result.status.completed()) {
        print_arm_capability_issues(snapshot.issues);
        return EXIT_FAILURE;
    }

    std::cout << "features:\n";
    print_coverage("floating_point", isa.floating_point);
    print_coverage("advanced_simd", isa.advanced_simd);
    print_coverage("fp16_scalar", isa.fp16_scalar);
    print_coverage("fp16_vector", isa.fp16_vector);
    print_coverage("bfloat16", isa.bfloat16);
    print_coverage("dot_product", isa.dot_product);
    print_coverage("i8mm", isa.i8mm);
    print_coverage("rdm", isa.rdm);
    print_coverage("fcma", isa.fcma);
    print_coverage("jscvt", isa.jscvt);
    print_coverage("atomics", isa.atomics);
    print_coverage("crc32", isa.crc32);
    print_coverage("flagm", isa.flagm);
    print_coverage("flagm2", isa.flagm2);
    print_coverage("aes", isa.aes);
    print_coverage("pmull", isa.pmull);
    print_coverage("sha1", isa.sha1);
    print_coverage("sha2", isa.sha2);
    print_coverage("sha3", isa.sha3);
    print_coverage("sm3", isa.sm3);
    print_coverage("sm4", isa.sm4);
    print_coverage("sve", isa.sve);
    print_coverage("sve2", isa.sve2);
    print_coverage("sve_aes", isa.sve_aes);
    print_coverage("sve_pmull", isa.sve_pmull);
    print_coverage("sve_bitperm", isa.sve_bitperm);
    print_coverage("sve_sha3", isa.sve_sha3);
    print_coverage("sve_sm4", isa.sve_sm4);
    print_coverage("sve_i8mm", isa.sve_i8mm);
    print_coverage("sve_bf16", isa.sve_bf16);

    std::cout << "sve_vector_length_bytes: ";
    if (isa.sve_vector_length_observed) {
        std::cout << isa.sve_vector_length_bytes;
    } else {
        std::cout << "unknown";
    }
    std::cout << '\n';

    print_arm_capability_issues(snapshot.issues);

    const arm::ArmProcessorIdentityQueryResult identity_result =
        arm::query_linux_arm_processor_identities(
            snapshot.target_logical_processor_ids);
    print_identity_groups(identity_result);

    return identity_result.status.completed()
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
