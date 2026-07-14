#include "cpu_capabilities.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <charconv>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace prometheus::backends::cpu {
namespace {

struct ProcCpuFeatureRecord {
    int os_cpu_id = -1;
    bool feature_line_present = false;
    std::set<std::string> features{};
};

[[nodiscard]] std::string trim_copy(std::string_view input) {
    std::size_t begin = 0;
    while (begin < input.size() &&
           std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
        ++begin;
    }

    std::size_t end = input.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
        --end;
    }
    return std::string(input.substr(begin, end - begin));
}

[[nodiscard]] std::optional<int> parse_int(std::string_view input) {
    const std::string trimmed = trim_copy(input);
    if (trimmed.empty()) {
        return std::nullopt;
    }

    int value = 0;
    const char* begin = trimmed.data();
    const char* end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::set<std::string> split_features(
    std::string_view input) {
    std::set<std::string> features;
    std::istringstream stream{std::string(input)};
    std::string feature;
    while (stream >> feature) {
        std::transform(
            feature.begin(),
            feature.end(),
            feature.begin(),
            [](unsigned char value) {
                return static_cast<char>(std::tolower(value));
            });
        features.insert(std::move(feature));
    }
    return features;
}

[[nodiscard]] std::optional<
    std::unordered_map<int, ProcCpuFeatureRecord>>
read_proc_cpu_features() {
    std::ifstream stream("/proc/cpuinfo");
    if (!stream) {
        return std::nullopt;
    }

    std::unordered_map<int, ProcCpuFeatureRecord> records;
    ProcCpuFeatureRecord current{};

    const auto commit = [&]() {
        if (current.os_cpu_id >= 0) {
            records[current.os_cpu_id] = current;
        }
    };

    std::string line;
    while (std::getline(stream, line)) {
        if (trim_copy(line).empty()) {
            commit();
            current = {};
            continue;
        }

        const std::size_t separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim_copy(
            std::string_view(line).substr(0, separator));
        const std::string value = trim_copy(
            std::string_view(line).substr(separator + 1));

        if (key == "processor") {
            const std::optional<int> parsed = parse_int(value);
            if (parsed.has_value()) {
                current.os_cpu_id = *parsed;
            }
        } else if (key == "flags" || key == "Features") {
            current.features = split_features(value);
            current.feature_line_present = true;
        }
    }
    commit();
    return records;
}

void add_issue(
    CpuCapabilitySnapshot& snapshot,
    CpuCapabilityIssueCode code,
    int os_cpu_id,
    bool fatal,
    std::string message) {
    CpuCapabilityIssue issue{};
    issue.code = code;
    issue.os_cpu_id = os_cpu_id;
    issue.fatal = fatal;
    issue.message = std::move(message);
    snapshot.issues.push_back(std::move(issue));
}

[[nodiscard]] CpuFeatureCoverage make_constant_coverage(
    std::size_t target_count,
    CpuSupportState state) {
    CpuFeatureCoverage coverage{};
    coverage.target_processor_count = target_count;
    coverage.observed_processor_count = target_count;
    coverage.supported_processor_count =
        state == CpuSupportState::supported ? target_count : 0;
    coverage.common = state;
    coverage.any = state;
    return coverage;
}

[[nodiscard]] bool has_any_feature(
    const ProcCpuFeatureRecord& record,
    std::initializer_list<const char*> names) {
    for (const char* name : names) {
        if (record.features.count(name) != 0) {
            return true;
        }
    }
    return false;
}

template <typename Predicate>
[[nodiscard]] CpuFeatureCoverage calculate_coverage(
    const std::vector<int>& target_ids,
    const std::unordered_map<int, ProcCpuFeatureRecord>& records,
    Predicate predicate) {
    CpuFeatureCoverage coverage{};
    coverage.target_processor_count = target_ids.size();

    for (const int cpu_id : target_ids) {
        const auto iterator = records.find(cpu_id);
        if (iterator == records.end() ||
            !iterator->second.feature_line_present) {
            continue;
        }

        ++coverage.observed_processor_count;
        if (predicate(iterator->second)) {
            ++coverage.supported_processor_count;
        }
    }

    if (coverage.target_processor_count == 0) {
        return coverage;
    }

    if (coverage.observed_processor_count == coverage.target_processor_count) {
        coverage.common =
            coverage.supported_processor_count == coverage.target_processor_count
                ? CpuSupportState::supported
                : CpuSupportState::unsupported;
        coverage.any =
            coverage.supported_processor_count > 0
                ? CpuSupportState::supported
                : CpuSupportState::unsupported;
    } else {
        coverage.common = CpuSupportState::unknown;
        coverage.any =
            coverage.supported_processor_count > 0
                ? CpuSupportState::supported
                : CpuSupportState::unknown;
    }

    return coverage;
}

[[nodiscard]] std::vector<int> select_target_processors(
    const CpuTopologySnapshot& topology) {
    std::vector<int> target_ids;

    const bool affinity_known = topology.summary.process_affinity_known;
    for (const CpuLogicalProcessorTopology& processor :
         topology.logical_processors) {
        if (affinity_known) {
            if (processor.process_allowed == CpuMembershipState::yes) {
                target_ids.push_back(processor.identity.os_cpu_id);
            }
        } else if (processor.online == CpuMembershipState::yes) {
            target_ids.push_back(processor.identity.os_cpu_id);
        }
    }

    std::sort(target_ids.begin(), target_ids.end());
    target_ids.erase(std::unique(target_ids.begin(), target_ids.end()),
                     target_ids.end());
    return target_ids;
}

[[nodiscard]] bool records_are_heterogeneous(
    const std::vector<int>& target_ids,
    const std::unordered_map<int, ProcCpuFeatureRecord>& records) {
    const std::set<std::string>* baseline = nullptr;
    for (const int cpu_id : target_ids) {
        const auto iterator = records.find(cpu_id);
        if (iterator == records.end() ||
            !iterator->second.feature_line_present) {
            continue;
        }

        if (baseline == nullptr) {
            baseline = &iterator->second.features;
        } else if (*baseline != iterator->second.features) {
            return true;
        }
    }
    return false;
}

void populate_common_simd_level(CpuIsaCapabilities& isa) {
    switch (isa.architecture) {
        case CpuArchitectureFamily::x86_64:
            if (isa.avx512f.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::avx512f;
                isa.common_vector_width_bytes = 64;
            } else if (isa.avx2.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::avx2;
                isa.common_vector_width_bytes = 32;
            } else if (isa.avx.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::avx;
                isa.common_vector_width_bytes = 32;
            } else if (isa.sse4_1.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::sse4_1;
                isa.common_vector_width_bytes = 16;
            } else if (isa.sse2.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::sse2;
                isa.common_vector_width_bytes = 16;
            }
            break;
        case CpuArchitectureFamily::aarch64:
        case CpuArchitectureFamily::armv7:
            if (isa.sve2.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::sve2;
                // SVE is vector-length agnostic. Width is resolved at runtime
                // by the future SIMD dispatch layer, so zero is intentional.
                isa.common_vector_width_bytes = 0;
            } else if (isa.sve.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::sve;
                isa.common_vector_width_bytes = 0;
            } else if (isa.neon.safe_for_all_targets()) {
                isa.common_simd_level = CpuSimdLevel::neon;
                isa.common_vector_width_bytes = 16;
            }
            break;
        case CpuArchitectureFamily::unknown:
        case CpuArchitectureFamily::riscv64:
        case CpuArchitectureFamily::ppc64le:
            break;
    }
}

}  // namespace

CpuArchitectureFamily compile_time_cpu_architecture() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    return CpuArchitectureFamily::x86_64;
#elif defined(__aarch64__)
    return CpuArchitectureFamily::aarch64;
#elif defined(__arm__)
    return CpuArchitectureFamily::armv7;
#elif defined(__riscv) && __riscv_xlen == 64
    return CpuArchitectureFamily::riscv64;
#elif defined(__powerpc64__) && defined(__LITTLE_ENDIAN__)
    return CpuArchitectureFamily::ppc64le;
#else
    return CpuArchitectureFamily::unknown;
#endif
}

CpuCapabilityQueryResult query_cpu_capabilities(
    const CpuTopologySnapshot& topology) {
    CpuCapabilityQueryResult result{};
    CpuCapabilitySnapshot& snapshot = result.capabilities;
    CpuIsaCapabilities& isa = snapshot.isa;

#if !defined(__linux__)
    result.status.code = CpuCapabilityQueryCode::unsupported_platform;
    result.status.message =
        "Phase 7.1 CPU capability discovery currently supports Linux only";
    return result;
#else
    isa.architecture = compile_time_cpu_architecture();
    isa.pointer_width_bits = sizeof(void*) * 8;

    const std::uint16_t endian_probe = 1;
    isa.little_endian =
        *reinterpret_cast<const std::uint8_t*>(&endian_probe) == 1;

    snapshot.target_logical_processor_ids =
        select_target_processors(topology);
    snapshot.target_logical_processor_count =
        snapshot.target_logical_processor_ids.size();

    if (snapshot.target_logical_processor_ids.empty()) {
        add_issue(
            snapshot,
            CpuCapabilityIssueCode::topology_unusable,
            -1,
            true,
            "no process-allowed or online logical processors are available");
        result.status.code = CpuCapabilityQueryCode::query_failed;
        result.status.message = "CPU capability query has no target processors";
        return result;
    }

    if (isa.architecture == CpuArchitectureFamily::unknown) {
        add_issue(
            snapshot,
            CpuCapabilityIssueCode::architecture_unknown,
            -1,
            false,
            "compile-time CPU architecture is unknown");
    }

    const std::optional<std::unordered_map<int, ProcCpuFeatureRecord>> records =
        read_proc_cpu_features();
    if (!records.has_value()) {
        add_issue(
            snapshot,
            CpuCapabilityIssueCode::proc_cpuinfo_unavailable,
            -1,
            false,
            "unable to read /proc/cpuinfo feature records");
    }

    const std::unordered_map<int, ProcCpuFeatureRecord> empty_records;
    const auto& feature_records = records.has_value() ? *records : empty_records;

    for (const int cpu_id : snapshot.target_logical_processor_ids) {
        const auto iterator = feature_records.find(cpu_id);
        if (iterator == feature_records.end()) {
            add_issue(
                snapshot,
                CpuCapabilityIssueCode::processor_record_missing,
                cpu_id,
                false,
                "processor record is missing from /proc/cpuinfo");
        } else if (!iterator->second.feature_line_present) {
            add_issue(
                snapshot,
                CpuCapabilityIssueCode::feature_record_missing,
                cpu_id,
                false,
                "processor feature line is missing from /proc/cpuinfo");
        }
    }

    isa.feature_sets_heterogeneous = records_are_heterogeneous(
        snapshot.target_logical_processor_ids,
        feature_records);

    const std::size_t target_count = snapshot.target_logical_processor_count;

    switch (isa.architecture) {
        case CpuArchitectureFamily::x86_64:
        case CpuArchitectureFamily::aarch64:
            isa.fp32 = make_constant_coverage(
                target_count, CpuSupportState::supported);
            isa.fp64 = make_constant_coverage(
                target_count, CpuSupportState::supported);
            break;
        default:
            isa.fp32 = calculate_coverage(
                snapshot.target_logical_processor_ids,
                feature_records,
                [](const ProcCpuFeatureRecord& record) {
                    return has_any_feature(
                        record, {"fpu", "vfp", "vfpv3", "vfpv4"});
                });
            isa.fp64 = calculate_coverage(
                snapshot.target_logical_processor_ids,
                feature_records,
                [](const ProcCpuFeatureRecord& record) {
                    return has_any_feature(
                        record, {"fp", "vfpv3", "vfpv4"});
                });
            break;
    }

    isa.fp16 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(
                record,
                {"avx512_fp16", "fphp", "asimdhp", "sve2p1"});
        });
    isa.bfloat16 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(
                record,
                {"avx512_bf16", "bf16", "svebf16", "sme2"});
        });

    isa.sse2 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sse2"});
        });
    isa.sse4_1 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sse4_1"});
        });
    isa.avx = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"avx"});
        });
    isa.avx2 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"avx2"});
        });
    isa.avx512f = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"avx512f"});
        });
    isa.fma = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"fma", "asimdfhm"});
        });
    isa.bmi1 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"bmi1", "bmi"});
        });
    isa.bmi2 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"bmi2"});
        });
    isa.popcnt = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"popcnt"});
        });

    isa.neon = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"asimd", "neon"});
        });
    isa.sve = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sve"});
        });
    isa.sve2 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sve2"});
        });
    isa.arm_atomics = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"atomics"});
        });
    isa.crc32 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"crc32", "sse4_2"});
        });

    isa.aes = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"aes"});
        });
    isa.sha1 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sha", "sha_ni", "sha1"});
        });
    isa.sha2 = calculate_coverage(
        snapshot.target_logical_processor_ids,
        feature_records,
        [](const ProcCpuFeatureRecord& record) {
            return has_any_feature(record, {"sha", "sha_ni", "sha2"});
        });

    isa.lock_free_atomic_u64 = make_constant_coverage(
        target_count,
        std::atomic<std::uint64_t>::is_always_lock_free
            ? CpuSupportState::supported
            : CpuSupportState::unsupported);

    populate_common_simd_level(isa);
    isa.source = records.has_value()
                     ? CpuCapabilitySource::combined
                     : CpuCapabilitySource::compile_time;

    const bool fatal_issue = std::any_of(
        snapshot.issues.begin(),
        snapshot.issues.end(),
        [](const CpuCapabilityIssue& issue) { return issue.fatal; });

    if (fatal_issue) {
        result.status.code = CpuCapabilityQueryCode::query_failed;
        result.status.message =
            "CPU capability discovery completed with a fatal issue";
    } else if (!snapshot.issues.empty()) {
        result.status.code = CpuCapabilityQueryCode::partial_success;
        result.status.message =
            "CPU capability discovery completed with partial information";
    } else {
        result.status.code = CpuCapabilityQueryCode::success;
    }

    return result;
#endif
}

const char* to_string(CpuArchitectureFamily value) noexcept {
    switch (value) {
        case CpuArchitectureFamily::unknown:
            return "unknown";
        case CpuArchitectureFamily::x86_64:
            return "x86_64";
        case CpuArchitectureFamily::aarch64:
            return "aarch64";
        case CpuArchitectureFamily::armv7:
            return "armv7";
        case CpuArchitectureFamily::riscv64:
            return "riscv64";
        case CpuArchitectureFamily::ppc64le:
            return "ppc64le";
    }
    return "unknown";
}

const char* to_string(CpuSupportState value) noexcept {
    switch (value) {
        case CpuSupportState::unknown:
            return "unknown";
        case CpuSupportState::unsupported:
            return "unsupported";
        case CpuSupportState::supported:
            return "supported";
    }
    return "unknown";
}

const char* to_string(CpuSimdLevel value) noexcept {
    switch (value) {
        case CpuSimdLevel::scalar:
            return "scalar";
        case CpuSimdLevel::sse2:
            return "sse2";
        case CpuSimdLevel::sse4_1:
            return "sse4_1";
        case CpuSimdLevel::avx:
            return "avx";
        case CpuSimdLevel::avx2:
            return "avx2";
        case CpuSimdLevel::avx512f:
            return "avx512f";
        case CpuSimdLevel::neon:
            return "neon";
        case CpuSimdLevel::sve:
            return "sve";
        case CpuSimdLevel::sve2:
            return "sve2";
    }
    return "scalar";
}

const char* to_string(CpuCapabilitySource value) noexcept {
    switch (value) {
        case CpuCapabilitySource::none:
            return "none";
        case CpuCapabilitySource::compile_time:
            return "compile_time";
        case CpuCapabilitySource::proc_cpuinfo:
            return "proc_cpuinfo";
        case CpuCapabilitySource::linux_auxv:
            return "linux_auxv";
        case CpuCapabilitySource::sysfs:
            return "sysfs";
        case CpuCapabilitySource::combined:
            return "combined";
    }
    return "none";
}

const char* to_string(CpuCapabilityIssueCode value) noexcept {
    switch (value) {
        case CpuCapabilityIssueCode::proc_cpuinfo_unavailable:
            return "proc_cpuinfo_unavailable";
        case CpuCapabilityIssueCode::processor_record_missing:
            return "processor_record_missing";
        case CpuCapabilityIssueCode::feature_record_missing:
            return "feature_record_missing";
        case CpuCapabilityIssueCode::architecture_unknown:
            return "architecture_unknown";
        case CpuCapabilityIssueCode::topology_unusable:
            return "topology_unusable";
    }
    return "unknown";
}

const char* to_string(CpuCapabilityQueryCode value) noexcept {
    switch (value) {
        case CpuCapabilityQueryCode::success:
            return "success";
        case CpuCapabilityQueryCode::partial_success:
            return "partial_success";
        case CpuCapabilityQueryCode::unsupported_platform:
            return "unsupported_platform";
        case CpuCapabilityQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::cpu
