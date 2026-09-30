#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/acs/acs_reference_wire.hpp"

namespace acs = prometheus::core::acs;

namespace {

constexpr std::uint16_t kPort = 39001;
constexpr int kPollMilliseconds = 250;
constexpr int kMaximumPolls = 60;
constexpr int kGracePolls = 4;
constexpr const char* kSchema = "public.transport.conformance.v1";
constexpr const char* kScope = "public.transport.conformance";

struct Profile {
    const char* node;
    const char* peer;
    const char* address;
    const char* peer_address;
};

constexpr Profile kProfiles[] = {
    {"node-001", "node-002", "10.77.0.1", "10.77.0.2"},
    {"node-002", "node-001", "10.77.0.2", "10.77.0.1"},
};

struct DirectionIds {
    acs::ParticipantId source{};
    acs::ParticipantId target{};
    acs::AuthorityId authority{};
    acs::CapabilityId capability{};
    acs::EvidenceId evidence{};
    acs::EndpointId source_endpoint{};
    acs::EndpointId target_endpoint{};
    acs::PortId source_port{};
    acs::PortId target_port{};
    acs::RelationshipId relationship{};
    acs::ConnectionId connection{};
    acs::BindingId binding{};
    acs::AttachmentId attachment{};
    acs::SignalId signal{};
    acs::AdmissionRequestId admission{};
};

struct DirectionContext {
    acs::AcsRegistrySnapshot registry{};
    acs::ConnectionStateSnapshot state{};
    acs::AcsAdmissionResult admission{};
    acs::TransportBindingSnapshot binding{};
    acs::AttachmentSnapshot attachment{};
    acs::SignalEnvelope envelope{};
};

int event_output_fd = STDOUT_FILENO;

void write_all(const char* data, std::size_t size) noexcept {
    while (size > 0) {
        const ssize_t count = write(event_output_fd, data, size);
        if (count > 0) {
            data += count;
            size -= static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return;
        }
    }
}

void emit(const char* record, const char* subject, const char* outcome,
          const char* detail) noexcept {
    std::array<char, 512> line{};
    const int length = std::snprintf(
        line.data(), line.size(),
        "{\"record\":\"%s\",\"subject\":\"%s\",\"outcome\":\"%s\","
        "\"detail\":\"%s\"}\n",
        record, subject, outcome, detail);
    if (length <= 0 || static_cast<std::size_t>(length) >= line.size()) return;
    write_all(line.data(), static_cast<std::size_t>(length));
}

template <typename Id>
std::optional<Id> make_id(const char* name_space, const std::string& value) {
    return Id::parse(name_space, value);
}

std::optional<DirectionIds> direction_ids(
    const std::string& source, const std::string& target) {
    DirectionIds ids{};
    const auto source_participant = make_id<acs::ParticipantId>("node", source);
    const auto target_participant = make_id<acs::ParticipantId>("node", target);
    const auto authority = make_id<acs::AuthorityId>(
        "authority", source + "-conformance");
    const auto capability = make_id<acs::CapabilityId>(
        "capability", source + "-conformance");
    const auto evidence = make_id<acs::EvidenceId>(
        "evidence", source + "-conformance");
    const auto source_endpoint = make_id<acs::EndpointId>(
        "endpoint", source + "-public");
    const auto target_endpoint = make_id<acs::EndpointId>(
        "endpoint", target + "-public");
    const auto source_port = make_id<acs::PortId>(
        "port", source + "-conformance");
    const auto target_port = make_id<acs::PortId>(
        "port", target + "-conformance");
    const auto relationship = make_id<acs::RelationshipId>(
        "relationship", "node-001-node-002-conformance");
    const auto connection = make_id<acs::ConnectionId>(
        "connection", source + "-to-" + target);
    const auto binding = make_id<acs::BindingId>(
        "binding", source + "-to-" + target + "-udp");
    const auto attachment = make_id<acs::AttachmentId>(
        "attachment", source + "-to-" + target);
    const auto signal = make_id<acs::SignalId>(
        "signal", source + "-to-" + target + "-conformance");
    const auto admission = make_id<acs::AdmissionRequestId>(
        "admission", source + "-to-" + target);
    if (!source_participant || !target_participant || !authority || !capability ||
        !evidence || !source_endpoint || !target_endpoint || !source_port ||
        !target_port || !relationship || !connection || !binding || !attachment ||
        !signal || !admission) {
        return std::nullopt;
    }
    ids.source = *source_participant;
    ids.target = *target_participant;
    ids.authority = *authority;
    ids.capability = *capability;
    ids.evidence = *evidence;
    ids.source_endpoint = *source_endpoint;
    ids.target_endpoint = *target_endpoint;
    ids.source_port = *source_port;
    ids.target_port = *target_port;
    ids.relationship = *relationship;
    ids.connection = *connection;
    ids.binding = *binding;
    ids.attachment = *attachment;
    ids.signal = *signal;
    ids.admission = *admission;
    return ids;
}

bool register_direction_prerequisites(
    acs::AcsRegistry& registry, const std::string& node) {
    const auto peer = node == "node-001" ? "node-002" : "node-001";
    const auto ids = direction_ids(node, peer);
    if (!ids) return false;
    return registry.register_authority(
               {ids->authority, ids->source, {kScope},
                acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() &&
           registry.register_capability(
               {ids->capability, ids->authority, ids->source, {kScope},
                acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() &&
           registry.register_evidence(
               {ids->evidence, ids->authority, ids->source, {kScope},
                acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok();
}

bool populate_registry(acs::AcsRegistry& registry) {
    const auto forward = direction_ids("node-001", "node-002");
    const auto reverse = direction_ids("node-002", "node-001");
    if (!forward || !reverse) return false;
    if (!registry.register_participant(
            {forward->source, acs::DescriptorRevision{1},
             acs::IdentityCondition::valid, true}).ok() ||
        !registry.register_participant(
            {forward->target, acs::DescriptorRevision{1},
             acs::IdentityCondition::valid, true}).ok() ||
        !register_direction_prerequisites(registry, "node-001") ||
        !register_direction_prerequisites(registry, "node-002") ||
        !registry.register_endpoint(
            {forward->source_endpoint, forward->source}).ok() ||
        !registry.register_endpoint(
            {forward->target_endpoint, forward->target}).ok()) {
        return false;
    }

    acs::PortDescriptor first_port{};
    first_port.id = forward->source_port;
    first_port.endpoint = forward->source_endpoint;
    first_port.purpose = {kScope};
    first_port.direction = acs::PortDirection::bidirectional;
    first_port.accepted_schemas = {{kSchema}};
    first_port.budgets = {{{"messages"}, acs::BudgetUnit::messages, 4}};
    first_port.failure_behavior = acs::FailureBehavior::defer;
    acs::PortDescriptor second_port = first_port;
    second_port.id = forward->target_port;
    second_port.endpoint = forward->target_endpoint;
    if (!registry.register_port(std::move(first_port)).ok() ||
        !registry.register_port(std::move(second_port)).ok() ||
        !registry.register_relationship(
            {forward->relationship, forward->source, forward->target,
             acs::RelationshipClass::infrastructure}).ok()) {
        return false;
    }

    acs::ConnectionDescriptor first_connection{};
    first_connection.id = forward->connection;
    first_connection.relationship = forward->relationship;
    first_connection.source_endpoint = forward->source_endpoint;
    first_connection.source_port = forward->source_port;
    first_connection.target_endpoint = forward->target_endpoint;
    first_connection.target_port = forward->target_port;
    first_connection.required_capabilities = {forward->capability};
    first_connection.required_evidence = {forward->evidence};
    acs::ConnectionDescriptor second_connection{};
    second_connection.id = reverse->connection;
    second_connection.relationship = reverse->relationship;
    second_connection.source_endpoint = reverse->source_endpoint;
    second_connection.source_port = reverse->source_port;
    second_connection.target_endpoint = reverse->target_endpoint;
    second_connection.target_port = reverse->target_port;
    second_connection.required_capabilities = {reverse->capability};
    second_connection.required_evidence = {reverse->evidence};
    return registry.register_connection(std::move(first_connection)).ok() &&
           registry.register_connection(std::move(second_connection)).ok();
}

std::optional<acs::TransitionId> transition_id(
    const DirectionIds& ids, const char* suffix) {
    return acs::TransitionId::parse(
        "transition", ids.connection.value() + "-" + suffix);
}

bool prepare_direction(
    acs::AcsRegistry& registry,
    acs::ConnectionStateStore& states,
    const std::string& source,
    const std::string& target,
    DirectionContext& context) {
    const auto ids = direction_ids(source, target);
    if (!ids) return false;
    const auto proposed_id = transition_id(*ids, "proposed");
    const auto nominal_id = transition_id(*ids, "nominal");
    const auto restricted_id = transition_id(*ids, "restricted");
    const auto unrestricted_id = transition_id(*ids, "unrestricted");
    const auto pending_id = transition_id(*ids, "pending");
    const auto admitted_id = transition_id(*ids, "admitted");
    const auto establishing_id = transition_id(*ids, "establishing");
    const auto active_id = transition_id(*ids, "active");
    if (!proposed_id || !nominal_id || !restricted_id || !unrestricted_id ||
        !pending_id || !admitted_id || !establishing_id || !active_id) {
        return false;
    }
    if (!states.transition_lifecycle(
            {*proposed_id, ids->connection, acs::LifecycleRevision{0},
             acs::ConnectionLifecycle::proposed, 0}).ok() ||
        !states.transition_operational(
            {*nominal_id, ids->connection, acs::OperationalRevision{0},
             acs::OperationalCondition::nominal,
             states.retained_idempotency_horizon()}).ok()) {
        return false;
    }
    acs::EnforcementTransitionRequest enforcement{};
    enforcement.transition_id = *restricted_id;
    enforcement.connection_id = ids->connection;
    enforcement.desired = acs::EnforcementCondition::restricted;
    enforcement.subject = ids->source;
    enforcement.scope = {kScope};
    enforcement.authority = ids->authority;
    enforcement.known_idempotency_horizon = states.retained_idempotency_horizon();
    if (!states.transition_enforcement(enforcement).ok()) return false;
    enforcement.transition_id = *unrestricted_id;
    enforcement.expected_revision = acs::EnforcementRevision{1};
    enforcement.desired = acs::EnforcementCondition::unrestricted;
    enforcement.capability = ids->capability;
    enforcement.admission_reference = ids->evidence;
    enforcement.known_idempotency_horizon = states.retained_idempotency_horizon();
    if (!states.transition_enforcement(enforcement).ok() ||
        !states.transition_lifecycle(
            {*pending_id, ids->connection, acs::LifecycleRevision{1},
             acs::ConnectionLifecycle::admission_pending,
             states.retained_idempotency_horizon()}).ok()) {
        return false;
    }

    const auto pending = states.find(ids->connection);
    if (!pending) return false;
    const auto snapshot = registry.snapshot();
    acs::AcsAdmissionRequest request{};
    request.request_id = ids->admission;
    request.connection_id = ids->connection;
    request.participant = ids->source;
    request.authority = ids->authority;
    request.expected_registry_generation = snapshot.generation;
    request.expected_connection_revision = acs::DescriptorRevision{1};
    request.identity_condition = acs::IdentityCondition::valid;
    request.evidence = {ids->evidence};
    request.capabilities = {ids->capability};
    request.state = *pending;
    request.execution_policy_outcome =
        prometheus::core::AdmissionOutcome::accepted;
    request.budgets = {{{"messages"}, acs::BudgetUnit::messages,
                        acs::BudgetUnit::messages, true, 1, 0, true, 4}};
    const auto admission = acs::evaluate_admission(snapshot, request);
    if (!admission.admitted()) return false;
    if (!states.transition_lifecycle(
            {*admitted_id, ids->connection, acs::LifecycleRevision{2},
             acs::ConnectionLifecycle::admitted,
             states.retained_idempotency_horizon()}).ok() ||
        !states.transition_lifecycle(
            {*establishing_id, ids->connection, acs::LifecycleRevision{3},
             acs::ConnectionLifecycle::establishing,
             states.retained_idempotency_horizon()}).ok() ||
        !states.transition_lifecycle(
            {*active_id, ids->connection, acs::LifecycleRevision{4},
             acs::ConnectionLifecycle::active,
             states.retained_idempotency_horizon()}).ok()) {
        return false;
    }
    const auto active = states.find(ids->connection);
    if (!active) return false;

    context.registry = snapshot;
    context.state = *active;
    context.admission = admission;
    context.binding.id = ids->binding;
    context.binding.connection_id = ids->connection;
    context.binding.source_endpoint = ids->source_endpoint;
    context.binding.target_endpoint = ids->target_endpoint;
    context.binding.transport_profile = {"isolated.udp.ipv4.v1"};
    context.binding.signal_schema = {kSchema};
    context.binding.signal_schema_version = 1;
    context.binding.profile_support = acs::TransportProfileSupport::supported;
    context.binding.condition = acs::BindingCondition::active;
    context.binding.revision = acs::DescriptorRevision{1};
    context.attachment.id = ids->attachment;
    context.attachment.connection_id = ids->connection;
    context.attachment.port_id = ids->target_port;
    context.attachment.participant = ids->source;
    context.attachment.authority = ids->authority;
    context.attachment.scope = {kScope};
    context.attachment.condition = acs::AttachmentCondition::active;
    context.attachment.revision = acs::DescriptorRevision{1};
    context.envelope.id = ids->signal;
    context.envelope.connection_id = ids->connection;
    context.envelope.binding_id = ids->binding;
    context.envelope.attachment_id = ids->attachment;
    context.envelope.source = ids->source;
    context.envelope.target = ids->target;
    context.envelope.source_endpoint = ids->source_endpoint;
    context.envelope.target_endpoint = ids->target_endpoint;
    context.envelope.source_port = ids->source_port;
    context.envelope.target_port = ids->target_port;
    context.envelope.domain = acs::SignalDomain::operational;
    context.envelope.intent = acs::SignalIntent::observation;
    context.envelope.subtype = {kScope};
    context.envelope.schema = {kSchema};
    context.envelope.schema_version = 1;
    context.envelope.freshness = acs::FreshnessCondition::current;
    context.envelope.provenance = {ids->evidence};
    const std::string payload = source + "-to-" + target;
    context.envelope.inline_value.assign(payload.begin(), payload.end());
    return true;
}

std::optional<Profile> read_profile() {
    FILE* input = std::fopen("/sys/class/dmi/id/product_serial", "r");
    if (!input) return std::nullopt;
    std::array<char, 64> value{};
    const bool read = std::fgets(value.data(), static_cast<int>(value.size()), input);
    const int close_status = std::fclose(input);
    if (!read || close_status != 0) return std::nullopt;
    value[std::strcspn(value.data(), "\r\n")] = '\0';
    for (const auto& profile : kProfiles) {
        if (std::strcmp(profile.node, value.data()) == 0) return profile;
    }
    return std::nullopt;
}

bool select_interface(int descriptor, std::array<char, IFNAMSIZ>& name) {
    ifreq flags{};
    std::snprintf(flags.ifr_name, sizeof(flags.ifr_name), "%s", "eth0");
    if (ioctl(descriptor, SIOCGIFFLAGS, &flags) != 0 ||
        (flags.ifr_flags & IFF_LOOPBACK) != 0) {
        return false;
    }
    std::snprintf(name.data(), name.size(), "%s", "eth0");
    return true;
}

bool configure_interface(
    int descriptor, const std::array<char, IFNAMSIZ>& name,
    const char* address) {
    sockaddr_in parsed{};
    parsed.sin_family = AF_INET;
    if (inet_pton(AF_INET, address, &parsed.sin_addr) != 1) return false;
    ifreq request{};
    std::snprintf(request.ifr_name, sizeof(request.ifr_name), "%s", name.data());
    std::memcpy(&request.ifr_addr, &parsed, sizeof(parsed));
    if (ioctl(descriptor, SIOCSIFADDR, &request) != 0) return false;
    parsed.sin_addr.s_addr = htonl(UINT32_C(0xffffff00));
    std::memcpy(&request.ifr_netmask, &parsed, sizeof(parsed));
    if (ioctl(descriptor, SIOCSIFNETMASK, &request) != 0 ||
        ioctl(descriptor, SIOCGIFFLAGS, &request) != 0) {
        return false;
    }
    request.ifr_flags = static_cast<short>(request.ifr_flags | IFF_UP);
    return ioctl(descriptor, SIOCSIFFLAGS, &request) == 0;
}

bool wire_self_test(
    const std::array<std::uint8_t, acs::kReferenceWireMaximumBytes>& encoded,
    std::size_t size) {
    acs::SignalEnvelope ignored{};
    if (acs::decode_reference_signal(encoded.data(), 8, ignored).code !=
        acs::ReferenceWireCode::truncated) {
        return false;
    }
    auto malformed = encoded;
    malformed[0] = 0;
    if (acs::decode_reference_signal(malformed.data(), size, ignored).code !=
        acs::ReferenceWireCode::invalid_header) {
        return false;
    }
    malformed = encoded;
    malformed[5] = 2;
    if (acs::decode_reference_signal(malformed.data(), size, ignored).code !=
        acs::ReferenceWireCode::unsupported_version) {
        return false;
    }
    malformed = encoded;
    malformed[8] = 0;
    malformed[9] = 1;
    malformed[10] = 0;
    malformed[11] = 0;
    return acs::decode_reference_signal(malformed.data(), size, ignored).code ==
           acs::ReferenceWireCode::oversized;
}

int run_transport(const Profile& profile) {
    acs::AcsRegistry registry{};
    if (!populate_registry(registry)) {
        emit("acs_transport_initialized", profile.node, "failed",
             "public_registry_initialization_failed");
        return 20;
    }
    acs::ConnectionStateStore states{registry};
    DirectionContext outgoing{};
    DirectionContext incoming{};
    if (!prepare_direction(registry, states, profile.node, profile.peer, outgoing) ||
        !prepare_direction(registry, states, profile.peer, profile.node, incoming)) {
        emit("acs_transport_initialized", profile.node, "failed",
             "public_contract_initialization_failed");
        return 21;
    }
    const auto outgoing_validation = acs::validate_transport_submission(
        outgoing.registry, outgoing.state, outgoing.admission, outgoing.binding,
        outgoing.attachment, outgoing.envelope);
    if (!outgoing_validation.valid()) {
        emit("acs_signal_validation_result", profile.node,
             acs::to_string(outgoing_validation.code),
             "sender_contract_validation_failed");
        return 22;
    }

    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> encoded{};
    const auto wire = acs::encode_reference_signal(
        outgoing.envelope, encoded.data(), encoded.size());
    if (!wire.ok()) {
        emit("acs_signal_send_result", profile.node, acs::to_string(wire.code),
             "wire_encoding_failed");
        return 23;
    }
    if (!wire_self_test(encoded, wire.bytes)) {
        emit("acs_wire_self_test", profile.node, "failed",
             "malformed_input_rejection_failed");
        return 24;
    }
    emit("acs_wire_self_test", profile.node, "passed",
         "invalid_header_truncated_oversized_and_version_rejected");

    const int descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (descriptor < 0) {
        emit("acs_transport_initialized", profile.node, "unavailable",
             "udp_socket_unavailable");
        return 0;
    }
    std::array<char, IFNAMSIZ> interface_name{};
    if (!select_interface(descriptor, interface_name) ||
        !configure_interface(descriptor, interface_name, profile.address)) {
        emit("acs_transport_initialized", profile.node, "unavailable",
             "managed_interface_unavailable");
        (void)close(descriptor);
        return 0;
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(kPort);
    sockaddr_in peer{};
    peer.sin_family = AF_INET;
    peer.sin_port = htons(kPort);
    if (inet_pton(AF_INET, profile.address, &local.sin_addr) != 1 ||
        inet_pton(AF_INET, profile.peer_address, &peer.sin_addr) != 1 ||
        bind(descriptor, reinterpret_cast<const sockaddr*>(&local),
             sizeof(local)) != 0) {
        emit("acs_transport_initialized", profile.node, "failed",
             "static_address_bind_failed");
        (void)close(descriptor);
        return 25;
    }
    emit("acs_network_configuration", profile.node, "configured",
         profile.address);
    emit("acs_transport_initialized", profile.node, "ready",
         "isolated_development_udp_not_production_secure");
    emit("acs_transport_binding", profile.node, "active",
         "isolated.udp.ipv4.v1");
    emit("acs_transport_attachment", profile.node, "active",
         "public_conformance_attachment_validated");
    emit("acs_signal_send_attempt", profile.node, "attempted",
         "bounded_public_conformance_signal");

    bool sent = false;
    bool received = false;
    int grace = 0;
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> received_bytes{};
    for (int attempt = 0; attempt < kMaximumPolls; ++attempt) {
        const ssize_t send_count = sendto(
            descriptor, encoded.data(), wire.bytes, 0,
            reinterpret_cast<const sockaddr*>(&peer), sizeof(peer));
        if (send_count == static_cast<ssize_t>(wire.bytes) && !sent) {
            sent = true;
            emit("acs_signal_send_result", profile.node, "transmitted",
                 "udp_datagram_submitted_no_delivery_claim");
        }
        pollfd wait{};
        wait.fd = descriptor;
        wait.events = POLLIN;
        const int poll_result = poll(&wait, 1, kPollMilliseconds);
        if (poll_result > 0 && (wait.revents & POLLIN) != 0) {
            sockaddr_in source{};
            iovec vector{received_bytes.data(), received_bytes.size()};
            msghdr message{};
            message.msg_name = &source;
            message.msg_namelen = sizeof(source);
            message.msg_iov = &vector;
            message.msg_iovlen = 1;
            const ssize_t count = recvmsg(descriptor, &message, MSG_TRUNC);
            if (count < 0) continue;
            if (static_cast<std::size_t>(count) > received_bytes.size()) {
                emit("acs_signal_receive", profile.node, "oversized",
                     "datagram_exceeded_reference_wire_bound");
                continue;
            }
            if (source.sin_addr.s_addr != peer.sin_addr.s_addr ||
                source.sin_port != peer.sin_port) {
                emit("acs_signal_receive", profile.node, "rejected",
                     "unexpected_development_peer_endpoint");
                continue;
            }
            acs::SignalEnvelope decoded{};
            const auto decoded_result = acs::decode_reference_signal(
                received_bytes.data(), static_cast<std::size_t>(count), decoded);
            if (!decoded_result.ok()) {
                emit("acs_signal_receive", profile.node,
                     acs::to_string(decoded_result.code),
                     "wire_validation_failed");
                continue;
            }
            emit("acs_signal_receive", profile.node, "complete",
                 "bounded_envelope_received");
            const auto validation = acs::validate_transport_submission(
                incoming.registry, incoming.state, incoming.admission,
                incoming.binding, incoming.attachment, decoded);
            emit("acs_signal_validation_result", profile.node,
                 acs::to_string(validation.code),
                 validation.valid() ? "public_contract_validation_complete"
                                    : "public_contract_validation_failed");
            if (validation.valid() && !received) {
                received = true;
                grace = kGracePolls;
            }
        }
        if (received && --grace <= 0) break;
    }
    emit("acs_transport_closed", profile.node,
         received ? "closed" : "peer_unavailable",
         received ? "conformance_exchange_complete"
                  : "bounded_wait_expired_without_valid_signal");
    (void)close(descriptor);
    return 0;
}

}  // namespace

int main() {
    if (std::getenv("NODE_P01_HOST_ROOT") != nullptr) {
        emit("acs_transport_initialized", "host-test", "unavailable",
             "guest_networking_not_attempted_in_host_test");
        return 0;
    }
    const int serial = open(
        "/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial >= 0) event_output_fd = serial;
    const auto profile = read_profile();
    if (!profile) {
        emit("acs_transport_initialized", "unmanaged-node", "unavailable",
             "managed_node_profile_not_present");
        return 0;
    }
    try {
        return run_transport(*profile);
    } catch (...) {
        emit("acs_transport_closed", profile->node, "resource_exhausted",
             "exception_contained_at_service_boundary");
        return 26;
    }
}
