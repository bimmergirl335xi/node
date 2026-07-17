#ifndef SRC_CORE_ACS_ACS_TYPES_HPP
#define SRC_CORE_ACS_ACS_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace prometheus::core::acs {

inline constexpr std::size_t kDefaultIdentifierBytes = 128;
inline constexpr std::size_t kAbsoluteIdentifierBytes = 256;
inline constexpr std::size_t kDefaultDiagnosticBytes = 512;
inline constexpr std::size_t kAbsoluteDiagnosticBytes = 2048;
inline constexpr std::size_t kDefaultEvidenceReferences = 16;
inline constexpr std::size_t kAbsoluteEvidenceReferences = 64;
inline constexpr std::size_t kDefaultBudgetDimensions = 16;
inline constexpr std::size_t kAbsoluteBudgetDimensions = 64;

enum class ValueCode : std::uint8_t {
    success = 0,
    empty,
    too_long,
    invalid_namespace,
    invalid_value,
    overflow,
};

struct ValidationStatus {
    ValueCode code = ValueCode::success;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept { return code == ValueCode::success; }
};

[[nodiscard]] inline bool canonical_token(std::string_view value) noexcept {
    if (value.empty()) return false;
    bool previous_separator = false;
    for (const unsigned char ch : value) {
        const bool alpha = ch >= 'a' && ch <= 'z';
        const bool digit = ch >= '0' && ch <= '9';
        const bool separator = ch == '.' || ch == '_' || ch == '-';
        if (!alpha && !digit && !separator) return false;
        if (separator && previous_separator) return false;
        previous_separator = separator;
    }
    const char first = value.front();
    const char last = value.back();
    const auto separator = [](char ch) { return ch == '.' || ch == '_' || ch == '-'; };
    return !separator(first) && !separator(last);
}

[[nodiscard]] inline ValidationStatus validate_bounded_text(
    std::string_view value,
    std::size_t maximum,
    bool require_canonical = true) {
    if (value.empty()) return {ValueCode::empty, "value is empty"};
    if (maximum == 0 || maximum > kAbsoluteIdentifierBytes || value.size() > maximum)
        return {ValueCode::too_long, "value exceeds its bound"};
    if (require_canonical && !canonical_token(value))
        return {ValueCode::invalid_value, "value is not canonical"};
    return {};
}

template <typename Tag>
class TypedId {
public:
    TypedId() = default;

    [[nodiscard]] static std::optional<TypedId> parse(
        std::string name_space,
        std::string value,
        std::size_t maximum_bytes = kDefaultIdentifierBytes) {
        if (!canonical_token(name_space) || !canonical_token(value) ||
            maximum_bytes == 0 || maximum_bytes > kAbsoluteIdentifierBytes ||
            name_space.size() >= maximum_bytes ||
            value.size() > maximum_bytes - name_space.size() - 1U) return std::nullopt;
        return TypedId{std::move(name_space), std::move(value)};
    }

    [[nodiscard]] const std::string& name_space() const noexcept { return namespace_; }
    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    [[nodiscard]] std::string canonical() const {
        return valid() ? namespace_ + ":" + value_ : std::string{};
    }
    [[nodiscard]] bool valid() const noexcept {
        return !namespace_.empty() && !value_.empty();
    }

    friend bool operator==(const TypedId& lhs, const TypedId& rhs) noexcept {
        return lhs.namespace_ == rhs.namespace_ && lhs.value_ == rhs.value_;
    }
    friend bool operator!=(const TypedId& lhs, const TypedId& rhs) noexcept {
        return !(lhs == rhs);
    }
    friend bool operator<(const TypedId& lhs, const TypedId& rhs) noexcept {
        return lhs.namespace_ < rhs.namespace_ ||
               (lhs.namespace_ == rhs.namespace_ && lhs.value_ < rhs.value_);
    }

private:
    TypedId(std::string name_space, std::string value)
        : namespace_(std::move(name_space)), value_(std::move(value)) {}
    std::string namespace_{};
    std::string value_{};
};

#define PROMETHEUS_ACS_ID(Name) struct Name##Tag {}; using Name = TypedId<Name##Tag>
PROMETHEUS_ACS_ID(ParticipantId);
PROMETHEUS_ACS_ID(AuthorityId);
PROMETHEUS_ACS_ID(CapabilityId);
PROMETHEUS_ACS_ID(DelegationId);
PROMETHEUS_ACS_ID(RevocationId);
PROMETHEUS_ACS_ID(EvidenceId);
PROMETHEUS_ACS_ID(EndpointId);
PROMETHEUS_ACS_ID(PortId);
PROMETHEUS_ACS_ID(RelationshipId);
PROMETHEUS_ACS_ID(ConnectionId);
PROMETHEUS_ACS_ID(TransitionId);
PROMETHEUS_ACS_ID(AdmissionRequestId);
PROMETHEUS_ACS_ID(RestrictionId);
PROMETHEUS_ACS_ID(ContainmentAuthorizationId);
PROMETHEUS_ACS_ID(RestorationClearanceId);
#undef PROMETHEUS_ACS_ID

template <typename Tag>
class MonotonicValue {
public:
    constexpr MonotonicValue() noexcept = default;
    explicit constexpr MonotonicValue(std::uint64_t value) noexcept : value_(value) {}
    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return value_; }
    [[nodiscard]] std::optional<MonotonicValue> next() const noexcept {
        if (value_ == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
        return MonotonicValue{value_ + 1U};
    }
    friend constexpr bool operator==(MonotonicValue a, MonotonicValue b) noexcept {
        return a.value_ == b.value_;
    }
    friend constexpr bool operator!=(MonotonicValue a, MonotonicValue b) noexcept {
        return !(a == b);
    }
private:
    std::uint64_t value_ = 0;
};

struct DescriptorRevisionTag {};
struct RegistryGenerationTag {};
struct LifecycleRevisionTag {};
struct OperationalRevisionTag {};
struct EnforcementRevisionTag {};
using DescriptorRevision = MonotonicValue<DescriptorRevisionTag>;
using RegistryGeneration = MonotonicValue<RegistryGenerationTag>;
using LifecycleRevision = MonotonicValue<LifecycleRevisionTag>;
using OperationalRevision = MonotonicValue<OperationalRevisionTag>;
using EnforcementRevision = MonotonicValue<EnforcementRevisionTag>;

enum class ConnectionLifecycle : std::uint8_t {
    unknown = 0, proposed, admission_pending, admitted, establishing,
    active, closing, closed, failed,
};
enum class OperationalCondition : std::uint8_t {
    unknown = 0, nominal, degraded, unavailable, failed,
};
enum class EnforcementCondition : std::uint8_t {
    unknown = 0, unrestricted, restricted, quarantined, revoked,
};
enum class AuthorityCondition : std::uint8_t {
    unknown = 0, valid, stale, expired, suspended, revoked, conflicting, unavailable,
};
enum class IdentityCondition : std::uint8_t {
    unknown = 0, valid, stale, conflicting, unavailable,
};
enum class FreshnessCondition : std::uint8_t {
    unknown = 0, current, stale, expired, conflicting, unavailable,
};
enum class PublicReferenceCondition : std::uint8_t {
    unknown = 0, current, restricted, contained, cleared, revoked, conflicting, unavailable,
};

struct ScopeReference {
    std::string value{};
    [[nodiscard]] bool valid(std::size_t maximum = kDefaultIdentifierBytes) const {
        return validate_bounded_text(value, maximum).ok();
    }
};

struct EvidenceReference {
    EvidenceId id{};
    AuthorityId issuer{};
    ParticipantId subject{};
    ScopeReference scope{};
    DescriptorRevision revision{};
    FreshnessCondition freshness = FreshnessCondition::unknown;
};
struct ImmuneObservationReference : EvidenceReference {};
struct ImmuneAssessmentReference : EvidenceReference {};
struct RestrictionReference {
    RestrictionId id{};
    AuthorityId issuer{};
    ParticipantId subject{};
    ScopeReference scope{};
    PublicReferenceCondition condition = PublicReferenceCondition::unknown;
    std::vector<EvidenceId> evidence{};
    DescriptorRevision revision{};
    FreshnessCondition freshness = FreshnessCondition::unknown;
};
struct ContainmentAuthorizationReference {
    ContainmentAuthorizationId id{};
    AuthorityId issuer{};
    ParticipantId subject{};
    ScopeReference scope{};
    PublicReferenceCondition condition = PublicReferenceCondition::unknown;
    DescriptorRevision revision{};
    FreshnessCondition freshness = FreshnessCondition::unknown;
};
struct RestorationClearanceReference {
    RestorationClearanceId id{};
    AuthorityId issuer{};
    ParticipantId subject{};
    ScopeReference scope{};
    PublicReferenceCondition condition = PublicReferenceCondition::unknown;
    std::vector<EvidenceId> evidence{};
    DescriptorRevision revision{};
    FreshnessCondition freshness = FreshnessCondition::unknown;
};

[[nodiscard]] inline bool checked_add(
    std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& result) noexcept {
    if (rhs > std::numeric_limits<std::uint64_t>::max() - lhs) return false;
    result = lhs + rhs;
    return true;
}
[[nodiscard]] inline bool checked_multiply(
    std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& result) noexcept {
    if (lhs != 0 && rhs > std::numeric_limits<std::uint64_t>::max() / lhs) return false;
    result = lhs * rhs;
    return true;
}

}  // namespace prometheus::core::acs

#endif
