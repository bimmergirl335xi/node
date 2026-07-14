#include "arm_processor_identity.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <utility>

namespace prometheus::backends::arm {
namespace {

struct RawProcessorRecord {
    int os_cpu_id = -1;
    bool processor_observed = false;

    bool implementer_observed = false;
    bool architecture_observed = false;
    bool variant_observed = false;
    bool part_observed = false;
    bool revision_observed = false;

    std::uint32_t implementer = 0;
    std::uint32_t architecture = 0;
    std::uint32_t variant = 0;
    std::uint32_t part = 0;
    std::uint32_t revision = 0;
};

[[nodiscard]] std::string_view trim(std::string_view value) noexcept {
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() &&
           std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return value;
}

[[nodiscard]] std::string lower_copy(std::string_view value) {
    std::string result{};
    result.reserve(value.size());
    for (const char character : value) {
        result.push_back(static_cast<char>(
            std::tolower(static_cast<unsigned char>(character))));
    }
    return result;
}

[[nodiscard]] bool parse_u32(
    std::string_view text,
    std::uint32_t& value) noexcept {
    text = trim(text);
    if (text.empty()) {
        return false;
    }

    int base = 10;
    if (text.size() > 2 && text[0] == '0' &&
        (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        text.remove_prefix(2);
    }

    if (text.empty()) {
        return false;
    }

    std::uint32_t parsed = 0;
    const auto [end, error] = std::from_chars(
        text.data(), text.data() + text.size(), parsed, base);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return false;
    }

    value = parsed;
    return true;
}

[[nodiscard]] bool parse_processor_id(
    std::string_view text,
    int& value) noexcept {
    text = trim(text);
    if (text.empty()) {
        return false;
    }

    int parsed = -1;
    const auto [end, error] = std::from_chars(
        text.data(), text.data() + text.size(), parsed, 10);
    if (error != std::errc{} || end != text.data() + text.size() ||
        parsed < 0) {
        return false;
    }

    value = parsed;
    return true;
}

void add_issue(
    ArmProcessorIdentitySnapshot& snapshot,
    ArmProcessorIdentityIssueCode code,
    int os_cpu_id,
    bool fatal,
    std::string message) {
    ArmProcessorIdentityIssue issue{};
    issue.code = code;
    issue.os_cpu_id = os_cpu_id;
    issue.fatal = fatal;
    issue.message = std::move(message);
    snapshot.issues.push_back(std::move(issue));
}

[[nodiscard]] std::string hex_value(std::uint32_t value) {
    std::ostringstream stream{};
    stream << "0x" << std::hex << value;
    return stream.str();
}

[[nodiscard]] bool same_signature(
    const ArmProcessorIdentity& left,
    const ArmProcessorIdentity& right) noexcept {
    return left.implementer_observed == right.implementer_observed &&
           left.architecture_observed == right.architecture_observed &&
           left.variant_observed == right.variant_observed &&
           left.part_observed == right.part_observed &&
           left.revision_observed == right.revision_observed &&
           left.implementer == right.implementer &&
           left.architecture == right.architecture &&
           left.variant == right.variant &&
           left.part == right.part &&
           left.revision == right.revision;
}

[[nodiscard]] ArmProcessorIdentity make_identity(
    const RawProcessorRecord& record) {
    ArmProcessorIdentity identity{};
    identity.observed = true;
    identity.implementer_observed = record.implementer_observed;
    identity.architecture_observed = record.architecture_observed;
    identity.variant_observed = record.variant_observed;
    identity.part_observed = record.part_observed;
    identity.revision_observed = record.revision_observed;
    identity.implementer = record.implementer;
    identity.architecture = record.architecture;
    identity.variant = record.variant;
    identity.part = record.part;
    identity.revision = record.revision;

    if (record.implementer_observed) {
        identity.implementer_text =
            arm_implementer_name(record.implementer);
    }
    if (record.part_observed) {
        identity.part_text = arm_part_name(
            record.implementer_observed ? record.implementer : 0,
            record.part);
    }

    identity.logical_processor_ids.push_back(record.os_cpu_id);
    return identity;
}

void parse_record_line(
    RawProcessorRecord& record,
    std::string_view key,
    std::string_view value,
    ArmProcessorIdentitySnapshot& snapshot) {
    const std::string normalized_key = lower_copy(trim(key));
    value = trim(value);

    if (normalized_key == "processor") {
        int processor_id = -1;
        if (!parse_processor_id(value, processor_id)) {
            add_issue(
                snapshot,
                ArmProcessorIdentityIssueCode::malformed_processor_id,
                -1,
                false,
                "processor record contains a malformed logical processor ID");
            return;
        }
        record.processor_observed = true;
        record.os_cpu_id = processor_id;
        return;
    }

    auto parse_identity_value = [&](bool& observed,
                                    std::uint32_t& destination,
                                    ArmProcessorIdentityIssueCode issue_code,
                                    const char* field_name) {
        std::uint32_t parsed = 0;
        if (!parse_u32(value, parsed)) {
            add_issue(
                snapshot,
                issue_code,
                record.processor_observed ? record.os_cpu_id : -1,
                false,
                std::string("processor record contains malformed ") +
                    field_name);
            return;
        }
        observed = true;
        destination = parsed;
    };

    if (normalized_key == "cpu implementer") {
        parse_identity_value(
            record.implementer_observed,
            record.implementer,
            ArmProcessorIdentityIssueCode::malformed_implementer,
            "CPU implementer");
    } else if (normalized_key == "cpu architecture") {
        parse_identity_value(
            record.architecture_observed,
            record.architecture,
            ArmProcessorIdentityIssueCode::malformed_architecture,
            "CPU architecture");
    } else if (normalized_key == "cpu variant") {
        parse_identity_value(
            record.variant_observed,
            record.variant,
            ArmProcessorIdentityIssueCode::malformed_variant,
            "CPU variant");
    } else if (normalized_key == "cpu part") {
        parse_identity_value(
            record.part_observed,
            record.part,
            ArmProcessorIdentityIssueCode::malformed_part,
            "CPU part");
    } else if (normalized_key == "cpu revision") {
        parse_identity_value(
            record.revision_observed,
            record.revision,
            ArmProcessorIdentityIssueCode::malformed_revision,
            "CPU revision");
    }
}

}  // namespace

ArmProcessorIdentityQueryResult parse_arm_proc_cpuinfo(
    std::string_view cpuinfo_text,
    const std::vector<int>& target_logical_processor_ids) {
    ArmProcessorIdentityQueryResult result{};
    ArmProcessorIdentitySnapshot& snapshot = result.snapshot;
    snapshot.source = ArmProcessorIdentitySource::injected_proc_cpuinfo;

    std::set<int> unique_targets{};
    for (const int processor_id : target_logical_processor_ids) {
        if (processor_id >= 0 && unique_targets.insert(processor_id).second) {
            snapshot.target_logical_processor_ids.push_back(processor_id);
        }
    }

    if (snapshot.target_logical_processor_ids.empty()) {
        add_issue(
            snapshot,
            ArmProcessorIdentityIssueCode::empty_target_set,
            -1,
            true,
            "ARM processor identity discovery requires target processors");
        result.status.code = ArmProcessorIdentityQueryCode::query_failed;
        result.status.message = "target processor set is empty";
        return result;
    }

    std::map<int, RawProcessorRecord> records{};
    RawProcessorRecord current{};

    const auto finish_record = [&]() {
        if (!current.processor_observed) {
            current = RawProcessorRecord{};
            return;
        }

        const bool inserted = records.emplace(
            current.os_cpu_id, current).second;
        if (!inserted) {
            add_issue(
                snapshot,
                ArmProcessorIdentityIssueCode::duplicate_processor_record,
                current.os_cpu_id,
                false,
                "duplicate processor record retained the first observation");
        }
        current = RawProcessorRecord{};
    };

    std::size_t position = 0;
    while (position <= cpuinfo_text.size()) {
        const std::size_t line_end = cpuinfo_text.find('\n', position);
        const std::size_t end = line_end == std::string_view::npos
            ? cpuinfo_text.size()
            : line_end;
        std::string_view line = cpuinfo_text.substr(position, end - position);
        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        if (trim(line).empty()) {
            finish_record();
        } else {
            const std::size_t separator = line.find(':');
            if (separator != std::string_view::npos) {
                parse_record_line(
                    current,
                    line.substr(0, separator),
                    line.substr(separator + 1),
                    snapshot);
            }
        }

        if (line_end == std::string_view::npos) {
            break;
        }
        position = line_end + 1;
    }
    finish_record();

    for (const int target_id : snapshot.target_logical_processor_ids) {
        const auto record = records.find(target_id);
        if (record == records.end()) {
            snapshot.missing_target_logical_processor_ids.push_back(target_id);
            add_issue(
                snapshot,
                ArmProcessorIdentityIssueCode::target_processor_missing,
                target_id,
                false,
                "target logical processor has no /proc/cpuinfo record");
            continue;
        }

        snapshot.observed_target_logical_processor_ids.push_back(target_id);
        ArmProcessorIdentity identity = make_identity(record->second);

        if (!identity.signature_complete()) {
            add_issue(
                snapshot,
                ArmProcessorIdentityIssueCode::incomplete_identity,
                target_id,
                false,
                "processor identity is missing implementer or part information");
        }

        auto existing = std::find_if(
            snapshot.identities.begin(),
            snapshot.identities.end(),
            [&](const ArmProcessorIdentity& candidate) {
                return same_signature(candidate, identity);
            });

        if (existing == snapshot.identities.end()) {
            snapshot.identities.push_back(std::move(identity));
        } else {
            existing->logical_processor_ids.push_back(target_id);
        }
    }

    snapshot.heterogeneous = snapshot.identities.size() > 1;

    if (snapshot.identities.empty()) {
        result.status.code = ArmProcessorIdentityQueryCode::query_failed;
        result.status.message =
            "no target ARM processor identities were recovered";
    } else if (!snapshot.issues.empty()) {
        result.status.code = ArmProcessorIdentityQueryCode::partial_success;
        result.status.message =
            "ARM processor identities were recovered with explicit issues";
    } else {
        result.status.code = ArmProcessorIdentityQueryCode::success;
        result.status.message = "ARM processor identities were recovered";
    }

    return result;
}

ArmProcessorIdentityQueryResult query_linux_arm_processor_identities(
    const std::vector<int>& target_logical_processor_ids,
    const std::string& cpuinfo_path) {
    std::ifstream input(cpuinfo_path);
    if (!input) {
        ArmProcessorIdentityQueryResult result{};
        result.snapshot.source = ArmProcessorIdentitySource::proc_cpuinfo;
        result.snapshot.target_logical_processor_ids =
            target_logical_processor_ids;
        add_issue(
            result.snapshot,
            ArmProcessorIdentityIssueCode::cpuinfo_unavailable,
            -1,
            true,
            "unable to open ARM processor identity source: " + cpuinfo_path);
        result.status.code = ArmProcessorIdentityQueryCode::query_failed;
        result.status.message = "processor identity source is unavailable";
        return result;
    }

    std::ostringstream buffer{};
    buffer << input.rdbuf();
    ArmProcessorIdentityQueryResult result = parse_arm_proc_cpuinfo(
        buffer.str(), target_logical_processor_ids);
    result.snapshot.source = ArmProcessorIdentitySource::proc_cpuinfo;
    return result;
}

std::string arm_implementer_name(std::uint32_t implementer) {
    switch (implementer) {
        case 0x41:
            return "Arm";
        case 0x42:
            return "Broadcom";
        case 0x43:
            return "Cavium";
        case 0x46:
            return "Fujitsu";
        case 0x48:
            return "HiSilicon";
        case 0x4e:
            return "NVIDIA";
        case 0x50:
            return "Applied Micro";
        case 0x51:
            return "Qualcomm";
        case 0x53:
            return "Samsung";
        case 0x61:
            return "Apple";
        case 0xc0:
            return "Ampere Computing";
        default:
            return hex_value(implementer);
    }
}

std::string arm_part_name(
    std::uint32_t implementer,
    std::uint32_t part) {
    if (implementer == 0x41) {
        switch (part) {
            case 0xd03:
                return "Cortex-A53";
            case 0xd05:
                return "Cortex-A55";
            case 0xd07:
                return "Cortex-A57";
            case 0xd08:
                return "Cortex-A72";
            case 0xd09:
                return "Cortex-A73";
            case 0xd0a:
                return "Cortex-A75";
            case 0xd0b:
                return "Cortex-A76";
            case 0xd0c:
                return "Neoverse-N1";
            case 0xd0d:
                return "Cortex-A77";
            case 0xd40:
                return "Neoverse-V1";
            case 0xd41:
                return "Cortex-A78";
            case 0xd46:
                return "Cortex-A510";
            case 0xd47:
                return "Cortex-A710";
            case 0xd48:
                return "Cortex-X2";
            case 0xd49:
                return "Neoverse-N2";
            default:
                break;
        }
    }

    return hex_value(part);
}

const char* to_string(ArmProcessorIdentitySource value) noexcept {
    switch (value) {
        case ArmProcessorIdentitySource::none:
            return "none";
        case ArmProcessorIdentitySource::injected_proc_cpuinfo:
            return "injected_proc_cpuinfo";
        case ArmProcessorIdentitySource::proc_cpuinfo:
            return "proc_cpuinfo";
    }
    return "unknown";
}

const char* to_string(ArmProcessorIdentityIssueCode value) noexcept {
    switch (value) {
        case ArmProcessorIdentityIssueCode::empty_target_set:
            return "empty_target_set";
        case ArmProcessorIdentityIssueCode::cpuinfo_unavailable:
            return "cpuinfo_unavailable";
        case ArmProcessorIdentityIssueCode::malformed_processor_id:
            return "malformed_processor_id";
        case ArmProcessorIdentityIssueCode::duplicate_processor_record:
            return "duplicate_processor_record";
        case ArmProcessorIdentityIssueCode::target_processor_missing:
            return "target_processor_missing";
        case ArmProcessorIdentityIssueCode::malformed_implementer:
            return "malformed_implementer";
        case ArmProcessorIdentityIssueCode::malformed_architecture:
            return "malformed_architecture";
        case ArmProcessorIdentityIssueCode::malformed_variant:
            return "malformed_variant";
        case ArmProcessorIdentityIssueCode::malformed_part:
            return "malformed_part";
        case ArmProcessorIdentityIssueCode::malformed_revision:
            return "malformed_revision";
        case ArmProcessorIdentityIssueCode::incomplete_identity:
            return "incomplete_identity";
    }
    return "unknown";
}

const char* to_string(ArmProcessorIdentityQueryCode value) noexcept {
    switch (value) {
        case ArmProcessorIdentityQueryCode::success:
            return "success";
        case ArmProcessorIdentityQueryCode::partial_success:
            return "partial_success";
        case ArmProcessorIdentityQueryCode::query_failed:
            return "query_failed";
    }
    return "unknown";
}

}  // namespace prometheus::backends::arm
