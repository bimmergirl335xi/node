#ifndef SRC_CORE_ACS_ACS_LIFECYCLE_HPP
#define SRC_CORE_ACS_ACS_LIFECYCLE_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "acs_registry.hpp"

namespace prometheus::core::acs {

struct StateStoreOptions {
    std::size_t maximum_connections = 1024;
    std::size_t maximum_history_per_connection = 32;
    std::size_t maximum_idempotency_records = 1024;
    std::size_t maximum_diagnostic_bytes = kDefaultDiagnosticBytes;
};

enum class StateDimension : std::uint8_t { lifecycle = 0, operational, enforcement };
enum class TransitionCode : std::uint8_t {
    success = 0,
    invalid_configuration,
    invalid_request,
    connection_not_found,
    capacity_exhausted,
    revision_conflict,
    invalid_transition,
    transition_identity_conflict,
    idempotency_expired,
    authority_denied,
    missing_reference,
    active_revocation,
    revision_overflow,
    resource_exhausted,
};

struct StateHistoryEntry {
    TransitionId transition_id{};
    StateDimension dimension = StateDimension::lifecycle;
    std::uint8_t from_value = 0;
    std::uint8_t to_value = 0;
    std::uint64_t revision = 0;
};

struct ConnectionStateSnapshot {
    ConnectionId connection_id{};
    ConnectionLifecycle lifecycle = ConnectionLifecycle::unknown;
    OperationalCondition operational = OperationalCondition::unknown;
    EnforcementCondition enforcement = EnforcementCondition::unknown;
    LifecycleRevision lifecycle_revision{};
    OperationalRevision operational_revision{};
    EnforcementRevision enforcement_revision{};
    std::uint64_t state_generation = 0;
    std::uint64_t retained_idempotency_horizon = 0;
    std::vector<StateHistoryEntry> history{};
};

struct TransitionResult {
    TransitionCode code = TransitionCode::success;
    ConnectionStateSnapshot state{};
    std::string message{};
    bool diagnostic_truncated = false;
    bool replayed = false;
    [[nodiscard]] bool ok() const noexcept { return code == TransitionCode::success; }
};

struct LifecycleTransitionRequest {
    TransitionId transition_id{};
    ConnectionId connection_id{};
    LifecycleRevision expected_revision{};
    ConnectionLifecycle desired = ConnectionLifecycle::unknown;
    std::uint64_t known_idempotency_horizon = 0;
};
struct OperationalTransitionRequest {
    TransitionId transition_id{};
    ConnectionId connection_id{};
    OperationalRevision expected_revision{};
    OperationalCondition desired = OperationalCondition::unknown;
    std::uint64_t known_idempotency_horizon = 0;
};
struct EnforcementTransitionRequest {
    TransitionId transition_id{};
    ConnectionId connection_id{};
    EnforcementRevision expected_revision{};
    EnforcementCondition desired = EnforcementCondition::unknown;
    ParticipantId subject{};
    ScopeReference scope{};
    AuthorityId authority{};
    std::optional<CapabilityId> capability{};
    std::optional<EvidenceId> admission_reference{};
    std::optional<ContainmentAuthorizationId> containment_authorization{};
    std::optional<RestorationClearanceId> restoration_clearance{};
    std::uint64_t known_idempotency_horizon = 0;
};

class ConnectionStateStore {
public:
    explicit ConnectionStateStore(const AcsRegistry& registry, StateStoreOptions options = {});
    ~ConnectionStateStore();
    ConnectionStateStore(const ConnectionStateStore&) = delete;
    ConnectionStateStore& operator=(const ConnectionStateStore&) = delete;

    [[nodiscard]] TransitionResult transition_lifecycle(const LifecycleTransitionRequest& request) noexcept;
    [[nodiscard]] TransitionResult transition_operational(const OperationalTransitionRequest& request) noexcept;
    [[nodiscard]] TransitionResult transition_enforcement(const EnforcementTransitionRequest& request) noexcept;
    [[nodiscard]] std::optional<ConnectionStateSnapshot> find(const ConnectionId& id) const;
    [[nodiscard]] std::vector<ConnectionStateSnapshot> snapshots() const;
    [[nodiscard]] std::uint64_t retained_idempotency_horizon() const;

private:
    struct Record { ConnectionStateSnapshot state{}; };
    struct IdempotencyRecord {
        TransitionId id{};
        std::string fingerprint{};
        TransitionResult result{};
    };
    [[nodiscard]] TransitionResult failure(TransitionCode code, std::string message) const;
    [[nodiscard]] std::optional<TransitionResult> replay_or_conflict(
        const TransitionId& id, const std::string& fingerprint, std::uint64_t known_horizon) const;
    [[nodiscard]] TransitionResult commit_transition(
        ConnectionStateSnapshot state, TransitionId transition_id,
        std::string fingerprint, bool new_record);
    // Registry reads complete before state_mutex_ is acquired. This enforces
    // registry-before-state ordering and prevents reverse/nested acquisition.
    const AcsRegistry* registry_ = nullptr;
    StateStoreOptions options_{};
    bool options_valid_ = false;
    mutable std::mutex state_mutex_{};
    std::vector<Record> records_{};
    std::vector<IdempotencyRecord> idempotency_{};
    std::uint64_t state_generation_ = 0;
    std::uint64_t idempotency_horizon_ = 0;
};

}  // namespace prometheus::core::acs
#endif
