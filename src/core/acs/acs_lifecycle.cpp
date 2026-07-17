#include "acs_lifecycle.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <sstream>
#include <utility>

namespace prometheus::core::acs {

namespace {
bool valid_options(const StateStoreOptions& value) {
    return value.maximum_connections > 0 && value.maximum_connections <= kAbsoluteConnections &&
           value.maximum_history_per_connection > 0 && value.maximum_history_per_connection <= 256 &&
           value.maximum_idempotency_records > 0 && value.maximum_idempotency_records <= 8192 &&
           value.maximum_diagnostic_bytes > 0 && value.maximum_diagnostic_bytes <= kAbsoluteDiagnosticBytes;
}

bool lifecycle_allowed(ConnectionLifecycle from, ConnectionLifecycle to) {
    using L = ConnectionLifecycle;
    switch (from) {
        case L::unknown: return to == L::proposed;
        case L::proposed: return to == L::admission_pending;
        case L::admission_pending: return to == L::admitted || to == L::failed;
        case L::admitted: return to == L::establishing || to == L::closing;
        case L::establishing: return to == L::active || to == L::failed || to == L::closing;
        case L::active: return to == L::closing || to == L::failed;
        case L::closing: return to == L::closed || to == L::failed;
        case L::closed: case L::failed: return false;
    }
    return false;
}

bool operational_allowed(OperationalCondition from, OperationalCondition to) {
    using O = OperationalCondition;
    if (to == O::unknown || from == to || from == O::failed) return false;
    return from == O::unknown || from == O::nominal || from == O::degraded || from == O::unavailable;
}

bool enforcement_allowed(EnforcementCondition from, EnforcementCondition to) {
    using E = EnforcementCondition;
    if (from == E::revoked || from == to || to == E::unknown) return false;
    if (from == E::unknown) return to == E::restricted || to == E::quarantined || to == E::revoked;
    if (from == E::unrestricted) return to == E::restricted || to == E::quarantined || to == E::revoked;
    if (from == E::restricted) return to == E::quarantined || to == E::unrestricted || to == E::revoked;
    if (from == E::quarantined) return to == E::restricted || to == E::revoked;
    return false;
}

template <typename T>
const T* locate(const std::vector<T>& values, const decltype(T{}.id)& id) {
    const auto found = std::find_if(values.begin(), values.end(), [&](const auto& value) { return value.id == id; });
    return found == values.end() ? nullptr : &*found;
}

std::string fingerprint(const LifecycleTransitionRequest& r) {
    return "l|" + r.connection_id.canonical() + "|" + std::to_string(r.expected_revision.value()) + "|" + std::to_string(static_cast<unsigned>(r.desired));
}
std::string fingerprint(const OperationalTransitionRequest& r) {
    return "o|" + r.connection_id.canonical() + "|" + std::to_string(r.expected_revision.value()) + "|" + std::to_string(static_cast<unsigned>(r.desired));
}
std::string fingerprint(const EnforcementTransitionRequest& r) {
    return "e|" + r.connection_id.canonical() + "|" + std::to_string(r.expected_revision.value()) + "|" + std::to_string(static_cast<unsigned>(r.desired)) + "|" + r.subject.canonical() + "|" + r.scope.value + "|" + r.authority.canonical() + "|" + (r.capability ? r.capability->canonical() : "-") + "|" + (r.admission_reference ? r.admission_reference->canonical() : "-") + "|" + (r.containment_authorization ? r.containment_authorization->canonical() : "-") + "|" + (r.restoration_clearance ? r.restoration_clearance->canonical() : "-");
}
}  // namespace

ConnectionStateStore::ConnectionStateStore(const AcsRegistry& registry, StateStoreOptions options)
    : registry_(&registry), options_(std::move(options)), options_valid_(valid_options(options_)) {}

ConnectionStateStore::~ConnectionStateStore() = default;

TransitionResult ConnectionStateStore::failure(TransitionCode code, std::string message) const {
    TransitionResult result{}; result.code = code;
    if (message.size() > options_.maximum_diagnostic_bytes) {
        message.resize(options_.maximum_diagnostic_bytes); result.diagnostic_truncated = true;
    }
    result.message = std::move(message); return result;
}

std::optional<TransitionResult> ConnectionStateStore::replay_or_conflict(
    const TransitionId& id, const std::string& value, std::uint64_t known_horizon) const {
    const auto found = std::find_if(idempotency_.begin(), idempotency_.end(), [&](const auto& record) { return record.id == id; });
    if (found != idempotency_.end()) {
        if (found->fingerprint != value) return failure(TransitionCode::transition_identity_conflict, "transition identity payload conflict");
        TransitionResult replay = found->result; replay.replayed = true; return replay;
    }
    if (known_horizon < idempotency_horizon_)
        return failure(TransitionCode::idempotency_expired, "idempotency knowledge expired");
    return std::nullopt;
}

void ConnectionStateStore::retain_idempotency(
    TransitionId id, std::string value, const TransitionResult& result) {
    if (idempotency_.size() == options_.maximum_idempotency_records) {
        idempotency_.erase(idempotency_.begin());
        ++idempotency_horizon_;
    }
    idempotency_.push_back({std::move(id), std::move(value), result});
}

TransitionResult ConnectionStateStore::transition_lifecycle(const LifecycleTransitionRequest& request) {
    if (!options_valid_) return failure(TransitionCode::invalid_configuration, "invalid state-store options");
    if (!request.transition_id.valid() || !request.connection_id.valid()) return failure(TransitionCode::invalid_request, "invalid transition request");
    if (!registry_->contains_connection(request.connection_id)) return failure(TransitionCode::connection_not_found, "connection not registered");
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto fp = fingerprint(request);
    if (auto replay = replay_or_conflict(request.transition_id, fp, request.known_idempotency_horizon)) return *replay;
    auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) { return record.state.connection_id == request.connection_id; });
    if (found == records_.end()) {
        if (records_.size() >= options_.maximum_connections) return failure(TransitionCode::capacity_exhausted, "state capacity exhausted");
        records_.push_back({ConnectionStateSnapshot{request.connection_id}}); found = std::prev(records_.end());
    }
    auto& state = found->state;
    if (state.lifecycle_revision != request.expected_revision) return failure(TransitionCode::revision_conflict, "lifecycle revision conflict");
    if (!lifecycle_allowed(state.lifecycle, request.desired)) return failure(TransitionCode::invalid_transition, "illegal lifecycle transition");
    const auto next = state.lifecycle_revision.next();
    if (!next || state_generation_ == std::numeric_limits<std::uint64_t>::max()) return failure(TransitionCode::revision_overflow, "lifecycle revision overflow");
    const auto before = state.lifecycle; state.lifecycle = request.desired; state.lifecycle_revision = *next; ++state_generation_; state.state_generation = state_generation_;
    if (state.history.size() == options_.maximum_history_per_connection) state.history.erase(state.history.begin());
    state.history.push_back({request.transition_id, StateDimension::lifecycle, static_cast<std::uint8_t>(before), static_cast<std::uint8_t>(request.desired), next->value()});
    state.retained_idempotency_horizon = idempotency_horizon_;
    TransitionResult result{}; result.state = state; retain_idempotency(request.transition_id, fp, result); return result;
}

TransitionResult ConnectionStateStore::transition_operational(const OperationalTransitionRequest& request) {
    if (!options_valid_ || !request.transition_id.valid() || !request.connection_id.valid()) return failure(TransitionCode::invalid_request, "invalid operational request");
    if (!registry_->contains_connection(request.connection_id)) return failure(TransitionCode::connection_not_found, "connection not registered");
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto fp = fingerprint(request); if (auto replay = replay_or_conflict(request.transition_id, fp, request.known_idempotency_horizon)) return *replay;
    auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) { return record.state.connection_id == request.connection_id; });
    if (found == records_.end() || found->state.operational_revision != request.expected_revision) return failure(TransitionCode::revision_conflict, "operational revision conflict");
    auto& state = found->state; if (!operational_allowed(state.operational, request.desired)) return failure(TransitionCode::invalid_transition, "illegal operational transition");
    const auto next = state.operational_revision.next(); if (!next || state_generation_ == std::numeric_limits<std::uint64_t>::max()) return failure(TransitionCode::revision_overflow, "operational revision overflow");
    const auto before = state.operational; state.operational = request.desired; state.operational_revision = *next; ++state_generation_; state.state_generation = state_generation_;
    if (state.history.size() == options_.maximum_history_per_connection) state.history.erase(state.history.begin());
    state.history.push_back({request.transition_id, StateDimension::operational, static_cast<std::uint8_t>(before), static_cast<std::uint8_t>(request.desired), next->value()});
    state.retained_idempotency_horizon = idempotency_horizon_; TransitionResult result{}; result.state = state; retain_idempotency(request.transition_id, fp, result); return result;
}

TransitionResult ConnectionStateStore::transition_enforcement(const EnforcementTransitionRequest& request) {
    if (!options_valid_ || !request.transition_id.valid() || !request.connection_id.valid() || !request.subject.valid() || !request.scope.valid() || !request.authority.valid()) return failure(TransitionCode::invalid_request, "invalid enforcement request");
    const auto registry = registry_->snapshot();
    if (!locate(registry.connections, request.connection_id)) return failure(TransitionCode::connection_not_found, "connection not registered");
    const auto authority = locate(registry.authorities, request.authority);
    if (!authority || authority->condition != AuthorityCondition::valid) return failure(TransitionCode::authority_denied, "restoration or enforcement authority is not valid");
    const bool revoked = std::any_of(registry.revocations.begin(), registry.revocations.end(), [&](const auto& value) { return value.subject == request.subject && value.condition == AuthorityCondition::revoked; });
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto fp = fingerprint(request); if (auto replay = replay_or_conflict(request.transition_id, fp, request.known_idempotency_horizon)) return *replay;
    auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) { return record.state.connection_id == request.connection_id; });
    if (found == records_.end() || found->state.enforcement_revision != request.expected_revision) return failure(TransitionCode::revision_conflict, "enforcement revision conflict");
    auto& state = found->state; if (!enforcement_allowed(state.enforcement, request.desired)) return failure(TransitionCode::invalid_transition, "illegal enforcement transition");
    if (request.desired != EnforcementCondition::revoked && revoked) return failure(TransitionCode::active_revocation, "active revocation prevents restoration");
    if (request.desired == EnforcementCondition::quarantined) {
        const auto* containment = request.containment_authorization ? locate(registry.containment_authorizations, *request.containment_authorization) : nullptr;
        if (!containment || containment->issuer != request.authority || containment->subject != request.subject || containment->scope.value != request.scope.value || containment->condition != PublicReferenceCondition::current || containment->freshness != FreshnessCondition::current)
            return failure(TransitionCode::missing_reference, "current matching containment authorization required");
    }
    if (state.enforcement == EnforcementCondition::quarantined && request.desired == EnforcementCondition::restricted) {
        const auto* clearance = request.restoration_clearance ? locate(registry.restoration_clearances, *request.restoration_clearance) : nullptr;
        if (!clearance || clearance->issuer != request.authority || clearance->subject != request.subject || clearance->scope.value != request.scope.value || clearance->condition != PublicReferenceCondition::cleared || clearance->freshness != FreshnessCondition::current)
            return failure(TransitionCode::missing_reference, "current matching restoration clearance required");
    }
    if (state.enforcement == EnforcementCondition::restricted && request.desired == EnforcementCondition::unrestricted) {
        const auto* capability = request.capability ? locate(registry.capabilities, *request.capability) : nullptr;
        const auto* admission = request.admission_reference ? locate(registry.evidence, *request.admission_reference) : nullptr;
        if (!capability || capability->subject != request.subject || capability->condition != AuthorityCondition::valid || !admission || admission->subject != request.subject || admission->freshness != FreshnessCondition::current)
            return failure(TransitionCode::missing_reference, "current admission and capability references required");
    }
    const auto next = state.enforcement_revision.next(); if (!next || state_generation_ == std::numeric_limits<std::uint64_t>::max()) return failure(TransitionCode::revision_overflow, "enforcement revision overflow");
    const auto before = state.enforcement; state.enforcement = request.desired; state.enforcement_revision = *next; ++state_generation_; state.state_generation = state_generation_;
    if (state.history.size() == options_.maximum_history_per_connection) state.history.erase(state.history.begin());
    state.history.push_back({request.transition_id, StateDimension::enforcement, static_cast<std::uint8_t>(before), static_cast<std::uint8_t>(request.desired), next->value()});
    state.retained_idempotency_horizon = idempotency_horizon_; TransitionResult result{}; result.state = state; retain_idempotency(request.transition_id, fp, result); return result;
}

std::optional<ConnectionStateSnapshot> ConnectionStateStore::find(const ConnectionId& id) const {
    std::lock_guard<std::mutex> lock(state_mutex_); const auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) { return record.state.connection_id == id; });
    return found == records_.end() ? std::nullopt : std::optional<ConnectionStateSnapshot>{found->state};
}
std::vector<ConnectionStateSnapshot> ConnectionStateStore::snapshots() const {
    std::lock_guard<std::mutex> lock(state_mutex_); std::vector<ConnectionStateSnapshot> result; result.reserve(records_.size());
    for (const auto& record : records_) result.push_back(record.state);
    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return lhs.connection_id < rhs.connection_id; }); return result;
}
std::uint64_t ConnectionStateStore::retained_idempotency_horizon() const {
    std::lock_guard<std::mutex> lock(state_mutex_); return idempotency_horizon_;
}
}  // namespace prometheus::core::acs
