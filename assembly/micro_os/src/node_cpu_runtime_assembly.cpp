#include "node_cpu_runtime_assembly.hpp"

#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace node::boot {
namespace {

constexpr std::size_t kMaximumIdentityBytes = 128;
constexpr std::size_t kMaximumProvenanceLineBytes = 256;
constexpr std::size_t kMaximumProgramHeaders = 128;
constexpr std::size_t kMaximumSectionHeaders = 256;
constexpr std::size_t kMaximumSymbols = 65536;
constexpr std::size_t kMaximumStringTableBytes = 1024U * 1024U;
constexpr std::string_view kRequiredSymbol =
    "node_cpu_runtime_conformance_v1";

constexpr std::array<std::uint32_t, 64> kSha256Constants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

constexpr std::uint32_t rotate_right(std::uint32_t value,
                                     unsigned amount) noexcept {
    return (value >> amount) | (value << (32U - amount));
}

void sha256_block(std::array<std::uint32_t, 8>& state,
                  const std::uint8_t* block) noexcept {
    std::array<std::uint32_t, 64> words{};
    for (std::size_t index = 0; index < 16; ++index) {
        const std::size_t offset = index * 4U;
        words[index] = (static_cast<std::uint32_t>(block[offset]) << 24U) |
                       (static_cast<std::uint32_t>(block[offset + 1U]) << 16U) |
                       (static_cast<std::uint32_t>(block[offset + 2U]) << 8U) |
                       static_cast<std::uint32_t>(block[offset + 3U]);
    }
    for (std::size_t index = 16; index < words.size(); ++index) {
        const std::uint32_t first =
            rotate_right(words[index - 15U], 7U) ^
            rotate_right(words[index - 15U], 18U) ^
            (words[index - 15U] >> 3U);
        const std::uint32_t second =
            rotate_right(words[index - 2U], 17U) ^
            rotate_right(words[index - 2U], 19U) ^
            (words[index - 2U] >> 10U);
        words[index] = words[index - 16U] + first + words[index - 7U] + second;
    }

    std::uint32_t a = state[0];
    std::uint32_t b = state[1];
    std::uint32_t c = state[2];
    std::uint32_t d = state[3];
    std::uint32_t e = state[4];
    std::uint32_t f = state[5];
    std::uint32_t g = state[6];
    std::uint32_t h = state[7];
    for (std::size_t index = 0; index < words.size(); ++index) {
        const std::uint32_t sigma_one =
            rotate_right(e, 6U) ^ rotate_right(e, 11U) ^ rotate_right(e, 25U);
        const std::uint32_t choice = (e & f) ^ ((~e) & g);
        const std::uint32_t temporary_one =
            h + sigma_one + choice + kSha256Constants[index] + words[index];
        const std::uint32_t sigma_zero =
            rotate_right(a, 2U) ^ rotate_right(a, 13U) ^ rotate_right(a, 22U);
        const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t temporary_two = sigma_zero + majority;
        h = g;
        g = f;
        f = e;
        e = d + temporary_one;
        d = c;
        c = b;
        b = a;
        a = temporary_one + temporary_two;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

bool checked_region(std::size_t offset,
                    std::size_t count,
                    std::size_t item_size,
                    std::size_t total) noexcept {
    return item_size == 0 ||
           (count <= (total - (offset <= total ? offset : total)) / item_size &&
            offset <= total);
}

template <typename Type>
bool copy_object(const std::vector<std::uint8_t>& bytes,
                 std::size_t offset,
                 Type& output) noexcept {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(Type)) return false;
    std::memcpy(&output, bytes.data() + offset, sizeof(Type));
    return true;
}

bool valid_hex_digest(std::string_view value) noexcept {
    if (value.size() != 64) return false;
    for (const char character : value) {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f'))) return false;
    }
    return true;
}

bool parse_boolean(std::string_view value, bool& output) noexcept {
    if (value == "true") {
        output = true;
        return true;
    }
    if (value == "false") {
        output = false;
        return true;
    }
    return false;
}

bool parse_size(std::string_view value, std::size_t& output) noexcept {
    if (value.empty() || (value.size() > 1 && value.front() == '0')) return false;
    std::size_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
        return false;
    }
    output = parsed;
    return true;
}

bool set_provenance_field(std::string_view key,
                          std::string_view value,
                          CpuRuntimeProvenance& output,
                          std::array<bool, 17>& seen) {
    std::size_t index = 0;
    if (key == "schema") {
        index = 0;
        if (value != "node.cpu-runtime-candidate.v1") return false;
    } else if (key == "component") {
        index = 1;
        output.component_identity = value;
    } else if (key == "abi") {
        index = 2;
        output.abi_identity = value;
    } else if (key == "architecture") {
        index = 3;
        output.architecture = value;
    } else if (key == "profile") {
        index = 4;
        output.profile = value;
    } else if (key == "source_identity") {
        index = 5;
        output.source_identity = value;
    } else if (key == "source_revision") {
        index = 6;
        output.source_revision = value;
    } else if (key == "toolchain_identity") {
        index = 7;
        output.toolchain_identity = value;
    } else if (key == "configuration") {
        index = 8;
        output.configuration_identity = value;
    } else if (key == "compile_location") {
        index = 9;
        output.compile_location = value;
    } else if (key == "artifact_sha256") {
        index = 10;
        output.artifact_sha256 = value;
    } else if (key == "artifact_size") {
        index = 11;
        if (!parse_size(value, output.artifact_size)) return false;
    } else if (key == "parallelism") {
        index = 12;
        if (!parse_size(value, output.parallelism)) return false;
    } else if (key == "source_available") {
        index = 13;
        if (!parse_boolean(value, output.build.source_available)) return false;
    } else if (key == "toolchain_available") {
        index = 14;
        if (!parse_boolean(value, output.build.toolchain_available)) return false;
    } else if (key == "compile_result") {
        index = 15;
        if (value == "success") output.build.compile_succeeded = true;
        else if (value == "failure") output.build.compile_succeeded = false;
        else return false;
    } else if (key == "link_result") {
        index = 16;
        if (value == "success") output.build.link_succeeded = true;
        else if (value == "failure") output.build.link_succeeded = false;
        else return false;
    } else {
        return false;
    }
    if (seen[index]) return false;
    seen[index] = true;
    if (key != "artifact_sha256" && key != "artifact_size" &&
        key != "parallelism" && key != "source_available" &&
        key != "toolchain_available" && key != "compile_result" &&
        key != "link_result" && key != "schema" && !bounded_identity(value)) {
        return false;
    }
    return true;
}

bool contains_required_symbol(const std::vector<std::uint8_t>& bytes,
                              const Elf64_Ehdr& header) noexcept {
    if (header.e_shentsize != sizeof(Elf64_Shdr) || header.e_shnum == 0 ||
        header.e_shnum > kMaximumSectionHeaders ||
        !checked_region(header.e_shoff, header.e_shnum, sizeof(Elf64_Shdr),
                        bytes.size())) return false;
    std::vector<Elf64_Shdr> sections(header.e_shnum);
    for (std::size_t index = 0; index < sections.size(); ++index) {
        if (!copy_object(bytes, header.e_shoff + index * sizeof(Elf64_Shdr),
                         sections[index])) return false;
    }
    for (const Elf64_Shdr& symbols : sections) {
        if (symbols.sh_type != SHT_SYMTAB && symbols.sh_type != SHT_DYNSYM) continue;
        if (symbols.sh_entsize != sizeof(Elf64_Sym) ||
            symbols.sh_size % sizeof(Elf64_Sym) != 0 ||
            symbols.sh_link >= sections.size() ||
            symbols.sh_size / sizeof(Elf64_Sym) > kMaximumSymbols ||
            !checked_region(symbols.sh_offset, symbols.sh_size, 1, bytes.size())) {
            return false;
        }
        const Elf64_Shdr& strings = sections[symbols.sh_link];
        if (strings.sh_type != SHT_STRTAB || strings.sh_size == 0 ||
            strings.sh_size > kMaximumStringTableBytes ||
            !checked_region(strings.sh_offset, strings.sh_size, 1, bytes.size())) {
            return false;
        }
        const std::size_t count = symbols.sh_size / sizeof(Elf64_Sym);
        for (std::size_t index = 0; index < count; ++index) {
            Elf64_Sym symbol{};
            if (!copy_object(bytes,
                             symbols.sh_offset + index * sizeof(Elf64_Sym),
                             symbol)) return false;
            if (symbol.st_name >= strings.sh_size || symbol.st_shndx == SHN_UNDEF) {
                continue;
            }
            const char* name = reinterpret_cast<const char*>(
                bytes.data() + strings.sh_offset + symbol.st_name);
            const std::size_t remaining = strings.sh_size - symbol.st_name;
            const void* terminator = std::memchr(name, '\0', remaining);
            if (terminator == nullptr) return false;
            const std::size_t name_size =
                static_cast<const char*>(terminator) - name;
            if (std::string_view{name, name_size} == kRequiredSymbol &&
                (ELF64_ST_BIND(symbol.st_info) == STB_GLOBAL ||
                 ELF64_ST_BIND(symbol.st_info) == STB_WEAK)) return true;
        }
    }
    return false;
}

CpuRuntimeArtifactCode inspect_elf(const std::vector<std::uint8_t>& bytes) noexcept {
    Elf64_Ehdr header{};
    if (!copy_object(bytes, 0, header) ||
        std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0 ||
        header.e_ident[EI_CLASS] != ELFCLASS64 ||
        header.e_ident[EI_DATA] != ELFDATA2LSB ||
        header.e_ident[EI_VERSION] != EV_CURRENT ||
        header.e_version != EV_CURRENT || header.e_type != ET_EXEC ||
        header.e_machine != EM_X86_64 || header.e_entry == 0 ||
        header.e_ehsize != sizeof(Elf64_Ehdr) ||
        header.e_phentsize != sizeof(Elf64_Phdr) || header.e_phnum == 0 ||
        header.e_phnum > kMaximumProgramHeaders ||
        !checked_region(header.e_phoff, header.e_phnum, sizeof(Elf64_Phdr),
                        bytes.size())) {
        return CpuRuntimeArtifactCode::malformed;
    }
    for (std::size_t index = 0; index < header.e_phnum; ++index) {
        Elf64_Phdr program{};
        if (!copy_object(bytes, header.e_phoff + index * sizeof(Elf64_Phdr),
                         program)) return CpuRuntimeArtifactCode::malformed;
        if (program.p_type == PT_INTERP) return CpuRuntimeArtifactCode::incompatible;
        if (program.p_filesz > 0 &&
            !checked_region(program.p_offset, program.p_filesz, 1, bytes.size())) {
            return CpuRuntimeArtifactCode::malformed;
        }
    }
    return contains_required_symbol(bytes, header)
               ? CpuRuntimeArtifactCode::valid
               : CpuRuntimeArtifactCode::required_symbol_missing;
}

}  // namespace

bool bounded_identity(std::string_view value) noexcept {
    if (value.empty() || value.size() > kMaximumIdentityBytes) return false;
    for (const char character : value) {
        if (!((character >= 'a' && character <= 'z') ||
              (character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9') || character == '.' ||
              character == '_' || character == ':' || character == '+' ||
              character == '-' || character == '/')) return false;
    }
    return true;
}

CpuRuntimeBuildCode validate_cpu_runtime_build_request(
    const CpuRuntimeBuildRequest& request) noexcept {
    if (!bounded_identity(request.node_identity) ||
        !bounded_identity(request.generation_identity) ||
        !bounded_identity(request.component_identity) ||
        !bounded_identity(request.architecture) ||
        !bounded_identity(request.profile) ||
        !bounded_identity(request.source_identity) ||
        !bounded_identity(request.source_revision) ||
        !bounded_identity(request.toolchain_identity) ||
        !bounded_identity(request.configuration_identity) ||
        !bounded_identity(request.request_identity) ||
        !bounded_identity(request.attempt_identity)) {
        return CpuRuntimeBuildCode::invalid_request;
    }
    if (!request.component_required) {
        return CpuRuntimeBuildCode::required_component_missing;
    }
    if (!request.profile_compatible ||
        request.component_identity != kCpuRuntimeComponentIdentity ||
        request.generation_identity != kCpuRuntimeGenerationIdentity ||
        request.architecture != kCpuRuntimeArchitecture ||
        request.profile != kCpuRuntimeProfile ||
        request.source_identity != kCpuRuntimeSourceIdentity ||
        request.source_revision != kCpuRuntimeSourceRevision) {
        return CpuRuntimeBuildCode::invalid_request;
    }
    return CpuRuntimeBuildCode::success;
}

CpuRuntimeBuildCode evaluate_cpu_runtime_build(
    const CpuRuntimeBuildEvidence& evidence) noexcept {
    if (!evidence.source_available) return CpuRuntimeBuildCode::source_missing;
    if (!evidence.toolchain_available) {
        return CpuRuntimeBuildCode::toolchain_unavailable;
    }
    if (!evidence.compile_succeeded) return CpuRuntimeBuildCode::compile_failure;
    if (!evidence.link_succeeded) return CpuRuntimeBuildCode::link_failure;
    return CpuRuntimeBuildCode::success;
}

bool parse_cpu_runtime_provenance(std::string_view encoded,
                                  CpuRuntimeProvenance& output) noexcept {
    try {
        if (encoded.empty() || encoded.size() > kCpuRuntimeProvenanceMaximumBytes ||
            encoded.back() != '\n') return false;
        CpuRuntimeProvenance parsed{};
        std::array<bool, 17> seen{};
        std::size_t offset = 0;
        while (offset < encoded.size()) {
            const std::size_t end = encoded.find('\n', offset);
            if (end == std::string_view::npos || end == offset ||
                end - offset > kMaximumProvenanceLineBytes) return false;
            const std::string_view line = encoded.substr(offset, end - offset);
            const std::size_t separator = line.find('=');
            if (separator == std::string_view::npos || separator == 0 ||
                separator + 1 >= line.size()) return false;
            if (!set_provenance_field(line.substr(0, separator),
                                      line.substr(separator + 1), parsed, seen)) {
                return false;
            }
            offset = end + 1;
        }
        for (const bool present : seen) {
            if (!present) return false;
        }
        if (parsed.component_identity != kCpuRuntimeComponentIdentity ||
            parsed.abi_identity != kCpuRuntimeAbiIdentity ||
            parsed.source_identity != kCpuRuntimeSourceIdentity ||
            parsed.source_revision != kCpuRuntimeSourceRevision ||
            !valid_hex_digest(parsed.artifact_sha256) ||
            parsed.artifact_size == 0 ||
            parsed.artifact_size > kCpuRuntimeArtifactMaximumBytes ||
            parsed.parallelism != 1) return false;
        output = std::move(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

std::string sha256_hex(const std::uint8_t* data, std::size_t size) {
    std::array<std::uint32_t, 8> state{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U,
    };
    std::size_t offset = 0;
    while (size - offset >= 64U) {
        sha256_block(state, data + offset);
        offset += 64U;
    }
    std::array<std::uint8_t, 128> tail{};
    const std::size_t remaining = size - offset;
    if (remaining > 0) std::memcpy(tail.data(), data + offset, remaining);
    tail[remaining] = 0x80U;
    const std::size_t padded_size = remaining < 56U ? 64U : 128U;
    const std::uint64_t bit_size = static_cast<std::uint64_t>(size) * 8U;
    for (std::size_t index = 0; index < 8; ++index) {
        tail[padded_size - 1U - index] =
            static_cast<std::uint8_t>(bit_size >> (index * 8U));
    }
    sha256_block(state, tail.data());
    if (padded_size == 128U) sha256_block(state, tail.data() + 64U);

    static constexpr char digits[] = "0123456789abcdef";
    std::string result(64, '0');
    for (std::size_t index = 0; index < state.size(); ++index) {
        for (std::size_t byte = 0; byte < 4; ++byte) {
            const std::uint8_t value = static_cast<std::uint8_t>(
                state[index] >> ((3U - byte) * 8U));
            result[index * 8U + byte * 2U] = digits[value >> 4U];
            result[index * 8U + byte * 2U + 1U] = digits[value & 0x0fU];
        }
    }
    return result;
}

CpuRuntimeArtifactValidation validate_cpu_runtime_artifact(
    const std::string& path,
    const CpuRuntimeProvenance& provenance,
    std::string_view expected_architecture,
    std::string_view expected_profile) noexcept {
    CpuRuntimeArtifactValidation result{};
    try {
        if (provenance.architecture != expected_architecture ||
            provenance.profile != expected_profile ||
            expected_architecture != kCpuRuntimeArchitecture ||
            expected_profile != kCpuRuntimeProfile) {
            result.code = CpuRuntimeArtifactCode::incompatible;
            return result;
        }
        const int descriptor = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (descriptor < 0) return result;
        struct stat status {};
        if (fstat(descriptor, &status) != 0 || !S_ISREG(status.st_mode) ||
            status.st_size <= 0) {
            (void)close(descriptor);
            result.code = CpuRuntimeArtifactCode::malformed;
            return result;
        }
        result.size_bytes = static_cast<std::size_t>(status.st_size);
        if (result.size_bytes > kCpuRuntimeArtifactMaximumBytes) {
            (void)close(descriptor);
            result.code = CpuRuntimeArtifactCode::too_large;
            return result;
        }
        std::vector<std::uint8_t> bytes(result.size_bytes);
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t count = read(descriptor, bytes.data() + offset,
                                       bytes.size() - offset);
            if (count > 0) offset += static_cast<std::size_t>(count);
            else if (count < 0 && errno == EINTR) continue;
            else {
                (void)close(descriptor);
                result.code = CpuRuntimeArtifactCode::malformed;
                return result;
            }
        }
        (void)close(descriptor);
        if (result.size_bytes != provenance.artifact_size) {
            result.code = CpuRuntimeArtifactCode::malformed;
            return result;
        }
        result.sha256 = sha256_hex(bytes.data(), bytes.size());
        if (result.sha256 != provenance.artifact_sha256) {
            result.code = CpuRuntimeArtifactCode::digest_mismatch;
            return result;
        }
        result.code = inspect_elf(bytes);
        return result;
    } catch (...) {
        result.code = CpuRuntimeArtifactCode::malformed;
        return result;
    }
}

CpuRuntimeProcessCode evaluate_cpu_runtime_process(
    const CpuRuntimeProcessEvidence& evidence,
    std::uint64_t expected_probe_result) noexcept {
    if (!evidence.launched || !evidence.interface_observed) {
        return CpuRuntimeProcessCode::activation_failure;
    }
    if (evidence.timed_out) return CpuRuntimeProcessCode::probe_timeout;
    if (!evidence.exited_normally || evidence.exit_code != 0) {
        return CpuRuntimeProcessCode::runtime_process_failure;
    }
    if (!evidence.probe_output_observed ||
        evidence.probe_result != expected_probe_result) {
        return CpuRuntimeProcessCode::probe_incorrect_result;
    }
    return CpuRuntimeProcessCode::success;
}

const char* to_string(CpuRuntimeBuildCode value) noexcept {
    switch (value) {
        case CpuRuntimeBuildCode::success: return "success";
        case CpuRuntimeBuildCode::invalid_request: return "invalid_request";
        case CpuRuntimeBuildCode::required_component_missing:
            return "required_component_missing";
        case CpuRuntimeBuildCode::source_missing: return "source_missing";
        case CpuRuntimeBuildCode::toolchain_unavailable:
            return "toolchain_unavailable";
        case CpuRuntimeBuildCode::compile_failure: return "compile_failure";
        case CpuRuntimeBuildCode::link_failure: return "link_failure";
    }
    return "invalid_request";
}

const char* to_string(CpuRuntimeArtifactCode value) noexcept {
    switch (value) {
        case CpuRuntimeArtifactCode::valid: return "valid";
        case CpuRuntimeArtifactCode::missing: return "missing";
        case CpuRuntimeArtifactCode::malformed: return "malformed";
        case CpuRuntimeArtifactCode::incompatible: return "incompatible";
        case CpuRuntimeArtifactCode::too_large: return "too_large";
        case CpuRuntimeArtifactCode::digest_mismatch: return "digest_mismatch";
        case CpuRuntimeArtifactCode::required_symbol_missing:
            return "required_symbol_missing";
    }
    return "malformed";
}

const char* to_string(CpuRuntimeProcessCode value) noexcept {
    switch (value) {
        case CpuRuntimeProcessCode::success: return "success";
        case CpuRuntimeProcessCode::activation_failure:
            return "activation_failure";
        case CpuRuntimeProcessCode::runtime_process_failure:
            return "runtime_process_failure";
        case CpuRuntimeProcessCode::probe_timeout: return "probe_timeout";
        case CpuRuntimeProcessCode::probe_incorrect_result:
            return "probe_incorrect_result";
    }
    return "runtime_process_failure";
}

}  // namespace node::boot
