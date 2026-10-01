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
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/acs/acs_discovery_wire.hpp"
#include "core/acs/acs_reference_wire.hpp"

namespace acs = prometheus::core::acs;

namespace {

constexpr int kPollMilliseconds = 250;
constexpr int kMaximumPolls = 100;
constexpr int kGracePolls = 4;
constexpr std::uint64_t kDiscoveryAnnouncementMilliseconds = 2000;
constexpr std::uint64_t kDiscoveryStaleMilliseconds = 6000;
constexpr std::uint64_t kDiscoveryConformanceMilliseconds = 8000;
constexpr std::uint16_t kDiscoveryPort = 39002;
constexpr const char* kDiscoveryGroup = "239.77.0.1";
constexpr const char* kDiscoveryTransportProfile =
    "isolated.udp.ipv4.discovery.v1";
constexpr std::size_t kMaximumPeers = 16;
constexpr std::size_t kMaximumProfileBytes = 512;
constexpr const char* kSchema = "public.transport.conformance.v1";
constexpr const char* kScope = "public.transport.conformance";
constexpr const char* kResidentReadyPath =
    "/run/node-p01-results/acs_reference_transport.ready";
constexpr const char* kAcsModePath =
    "/run/node-p01-results/acs_reference_transport.mode";

struct Peer { std::string node; std::string address; };
enum class AcsMode : std::uint8_t { explicit_peers = 0, discovery };
struct Profile {
    std::string node;
    std::string address;
    std::uint16_t port = 0;
    std::vector<Peer> peers;
    AcsMode mode = AcsMode::explicit_peers;
};
struct DirectionIds {
    acs::ParticipantId source;
    acs::ParticipantId target;
    acs::AuthorityId authority;
    acs::CapabilityId capability;
    acs::EvidenceId evidence;
    acs::EndpointId source_endpoint;
    acs::EndpointId target_endpoint;
    acs::PortId source_port;
    acs::PortId target_port;
    acs::RelationshipId relationship;
    acs::ConnectionId connection;
    acs::BindingId binding;
    acs::AttachmentId attachment;
    acs::SignalId signal;
    acs::CorrelationId correlation;
    acs::AdmissionRequestId admission;
};
struct DirectionContext {
    acs::AcsRegistrySnapshot registry;
    acs::ConnectionStateSnapshot state;
    acs::AcsAdmissionResult admission;
    acs::TransportBindingSnapshot binding;
    acs::AttachmentSnapshot attachment;
    acs::SignalEnvelope envelope;
};
struct PeerContext {
    Peer peer;
    DirectionContext outgoing;
    DirectionContext incoming;
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> encoded{};
    std::size_t encoded_bytes = 0;
    sockaddr_in endpoint{};
    bool sent = false;
    bool received = false;
    bool duplicate_reported = false;
    bool resident_activity_reported = false;
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

void emit(const char* record, const std::string& local, const std::string& peer,
          const char* outcome, const std::string& signal,
          const std::string& correlation, const char* detail) noexcept {
    std::array<char, 1024> line{};
    const int length = std::snprintf(
        line.data(), line.size(),
        "{\"record\":\"%s\",\"subject\":\"%s\",\"local\":\"%s\","
        "\"peer\":\"%s\",\"outcome\":\"%s\",\"signal\":\"%s\","
        "\"correlation\":\"%s\",\"detail\":\"%s\"}\n",
        record, local.c_str(), local.c_str(), peer.c_str(), outcome,
        signal.c_str(), correlation.c_str(), detail);
    if (length > 0 && static_cast<std::size_t>(length) < line.size())
        write_all(line.data(), static_cast<std::size_t>(length));
}

void emit_local(const char* record, const std::string& local,
                const char* outcome, const char* detail) noexcept {
    emit(record, local, "", outcome, "", "", detail);
}

bool lab_mode() noexcept {
    const char* mode = std::getenv("NODE_MICRO_OS_MODE");
    return mode != nullptr && std::strcmp(mode, "lab") == 0;
}

const char* mode_name(AcsMode mode) noexcept {
    return mode == AcsMode::discovery ? "discovery" : "explicit";
}

bool write_exact_marker(const char* path, const char* value) noexcept {
    struct stat existing {};
    if (lstat(path, &existing) == 0) {
        if (!S_ISREG(existing.st_mode) || unlink(path) != 0) return false;
    } else if (errno != ENOENT) {
        return false;
    }
    const int descriptor = open(
        path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (descriptor < 0) return false;
    const std::size_t size = std::strlen(value);
    std::size_t offset = 0;
    while (offset < size) {
        const ssize_t count = write(descriptor, value + offset, size - offset);
        if (count > 0) offset += static_cast<std::size_t>(count);
        else if (count < 0 && errno == EINTR) continue;
        else break;
    }
    const int closed = close(descriptor);
    const bool success = offset == size && closed == 0;
    if (!success) (void)unlink(path);
    return success;
}

bool write_mode_marker(AcsMode mode) noexcept {
    return write_exact_marker(
        kAcsModePath,
        mode == AcsMode::discovery
            ? "schema=node.acs-mode.v1\nmode=discovery\n"
            : "schema=node.acs-mode.v1\nmode=explicit\n");
}

bool write_ready_marker() noexcept {
    constexpr char value[] =
        "schema=node.resident-service-ready.v1\n"
        "service=acs_reference_transport\n"
        "state=ready\n";
    return write_exact_marker(kResidentReadyPath, value);
}

std::string correlation(const acs::SignalEnvelope& envelope);

int remain_resident(const Profile& profile, int descriptor,
                    std::vector<PeerContext>& contexts) {
    if (!write_mode_marker(AcsMode::explicit_peers) || !write_ready_marker()) {
        (void)unlink(kAcsModePath);
        emit_local("acs_resident", profile.node, "failed",
                   "resident_readiness_marker_failed");
        return 27;
    }
    emit_local("acs_resident", profile.node, "active",
               "public_reference_transport_current_boot_lab_scope");
    sigset_t signals;
    (void)sigemptyset(&signals);
    (void)sigaddset(&signals, SIGTERM);
    (void)sigaddset(&signals, SIGINT);
    const int signal_descriptor = signalfd(
        -1, &signals, SFD_CLOEXEC | SFD_NONBLOCK);
    if (signal_descriptor < 0) {
        (void)unlink(kResidentReadyPath);
        (void)unlink(kAcsModePath);
        return 28;
    }
    bool stopped = false;
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> received{};
    while (!stopped) {
        std::array<pollfd, 2> waits{{
            {signal_descriptor, POLLIN, 0},
            {descriptor, POLLIN, 0},
        }};
        const int polled = poll(waits.data(), waits.size(), kPollMilliseconds);
        if (polled < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if ((waits[0].revents & POLLIN) != 0) {
            signalfd_siginfo information{};
            if (read(signal_descriptor, &information, sizeof(information)) ==
                static_cast<ssize_t>(sizeof(information))) {
                stopped = true;
            }
        }
        if ((waits[1].revents & POLLIN) != 0) {
            sockaddr_in source{};
            iovec vector{received.data(), received.size()};
            msghdr message{};
            message.msg_name = &source;
            message.msg_namelen = sizeof(source);
            message.msg_iov = &vector;
            message.msg_iovlen = 1;
            const ssize_t count = recvmsg(descriptor, &message, MSG_TRUNC);
            if (count <= 0 || static_cast<std::size_t>(count) > received.size()) {
                continue;
            }
            auto context = std::find_if(
                contexts.begin(), contexts.end(),
                [&source](const PeerContext& item) {
                    return source.sin_addr.s_addr == item.endpoint.sin_addr.s_addr &&
                           source.sin_port == item.endpoint.sin_port;
                });
            if (context == contexts.end()) continue;
            acs::SignalEnvelope decoded{};
            const auto decoded_result = acs::decode_reference_signal(
                received.data(), static_cast<std::size_t>(count), decoded);
            if (!decoded_result.ok()) continue;
            const auto validation = acs::validate_transport_submission(
                context->incoming.registry, context->incoming.state,
                context->incoming.admission, context->incoming.binding,
                context->incoming.attachment, decoded);
            if (!validation.valid()) continue;
            const ssize_t sent = sendto(
                descriptor, context->encoded.data(), context->encoded_bytes, 0,
                reinterpret_cast<const sockaddr*>(&context->endpoint),
                sizeof(context->endpoint));
            if (sent == static_cast<ssize_t>(context->encoded_bytes) &&
                !context->resident_activity_reported) {
                context->resident_activity_reported = true;
                emit("acs_resident_peer_activity", profile.node,
                     context->peer.node, "validated_and_replied",
                     decoded.id.canonical(), correlation(decoded),
                     "bounded_rejoin_conformance_response");
            }
        }
    }
    (void)close(signal_descriptor);
    (void)unlink(kResidentReadyPath);
    (void)unlink(kAcsModePath);
    emit_local("acs_resident", profile.node,
               stopped ? "stopped" : "failed",
               stopped ? "bounded_shutdown_complete" : "resident_poll_failed");
    return stopped ? 0 : 28;
}

template <typename Id>
std::optional<Id> make_id(const char* name_space, const std::string& value) {
    return Id::parse(name_space, value);
}

std::optional<DirectionIds> direction_ids(
    const std::string& source, const std::string& target) {
    DirectionIds ids{};
    const std::string& first = source < target ? source : target;
    const std::string& second = source < target ? target : source;
    const auto source_participant = make_id<acs::ParticipantId>("node", source);
    const auto target_participant = make_id<acs::ParticipantId>("node", target);
    const auto authority = make_id<acs::AuthorityId>("authority", source + "-conformance");
    const auto capability = make_id<acs::CapabilityId>("capability", source + "-conformance");
    const auto evidence = make_id<acs::EvidenceId>("evidence", source + "-conformance");
    const auto source_endpoint = make_id<acs::EndpointId>("endpoint", source + "-public");
    const auto target_endpoint = make_id<acs::EndpointId>("endpoint", target + "-public");
    const auto source_port = make_id<acs::PortId>("port", source + "-conformance");
    const auto target_port = make_id<acs::PortId>("port", target + "-conformance");
    const auto relationship = make_id<acs::RelationshipId>(
        "relationship", first + "-" + second + "-conformance");
    const auto connection = make_id<acs::ConnectionId>("connection", source + "-to-" + target);
    const auto binding = make_id<acs::BindingId>("binding", source + "-to-" + target + "-udp");
    const auto attachment = make_id<acs::AttachmentId>("attachment", source + "-to-" + target);
    const auto signal = make_id<acs::SignalId>("signal", source + "-to-" + target + "-conformance");
    const auto correlation = make_id<acs::CorrelationId>(
        "correlation", source + "-to-" + target + "-first-boot");
    const auto admission = make_id<acs::AdmissionRequestId>("admission", source + "-to-" + target);
    if (!source_participant || !target_participant || !authority || !capability ||
        !evidence || !source_endpoint || !target_endpoint || !source_port ||
        !target_port || !relationship || !connection || !binding || !attachment ||
        !signal || !correlation || !admission) return std::nullopt;
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
    ids.correlation = *correlation;
    ids.admission = *admission;
    return ids;
}

bool register_node(acs::AcsRegistry& registry, const std::string& node,
                   const std::string& other) {
    const auto ids = direction_ids(node, other);
    if (!ids ||
        !registry.register_participant({ids->source, acs::DescriptorRevision{1},
            acs::IdentityCondition::valid, true}).ok() ||
        !registry.register_authority({ids->authority, ids->source, {kScope},
            acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_capability({ids->capability, ids->authority, ids->source,
            {kScope}, acs::AuthorityCondition::valid, acs::DescriptorRevision{1}}).ok() ||
        !registry.register_evidence({ids->evidence, ids->authority, ids->source,
            {kScope}, acs::DescriptorRevision{1}, acs::FreshnessCondition::current}).ok() ||
        !registry.register_endpoint({ids->source_endpoint, ids->source}).ok()) return false;
    acs::PortDescriptor port{};
    port.id = ids->source_port;
    port.endpoint = ids->source_endpoint;
    port.purpose = {kScope};
    port.direction = acs::PortDirection::bidirectional;
    port.accepted_schemas = {{kSchema}};
    port.budgets = {{{"messages"}, acs::BudgetUnit::messages, 16}};
    port.failure_behavior = acs::FailureBehavior::defer;
    return registry.register_port(std::move(port)).ok();
}

bool populate_registry(acs::AcsRegistry& registry,
                       const std::vector<std::string>& nodes) {
    if (nodes.size() < 2 || nodes.size() > kMaximumPeers + 1U) return false;
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (!register_node(registry, nodes[index], nodes[index == 0 ? 1 : 0])) return false;
    }
    for (std::size_t first = 0; first < nodes.size(); ++first) {
        for (std::size_t second = first + 1; second < nodes.size(); ++second) {
            const auto ids = direction_ids(nodes[first], nodes[second]);
            if (!ids || !registry.register_relationship({ids->relationship, ids->source,
                    ids->target, acs::RelationshipClass::infrastructure}).ok()) return false;
        }
    }
    for (const auto& source : nodes) {
        for (const auto& target : nodes) {
            if (source == target) continue;
            const auto ids = direction_ids(source, target);
            if (!ids) return false;
            acs::ConnectionDescriptor connection{};
            connection.id = ids->connection;
            connection.relationship = ids->relationship;
            connection.source_endpoint = ids->source_endpoint;
            connection.source_port = ids->source_port;
            connection.target_endpoint = ids->target_endpoint;
            connection.target_port = ids->target_port;
            connection.required_capabilities = {ids->capability};
            connection.required_evidence = {ids->evidence};
            if (!registry.register_connection(std::move(connection)).ok()) return false;
        }
    }
    return true;
}

std::optional<acs::TransitionId> transition_id(
    const DirectionIds& ids, const char* suffix) {
    return acs::TransitionId::parse("transition", ids.connection.value() + "-" + suffix);
}

bool prepare_direction(acs::AcsRegistry& registry, acs::ConnectionStateStore& states,
                       const std::string& source, const std::string& target,
                       DirectionContext& context) {
    const auto ids = direction_ids(source, target);
    if (!ids) return false;
    const auto proposed = transition_id(*ids, "proposed");
    const auto nominal = transition_id(*ids, "nominal");
    const auto restricted = transition_id(*ids, "restricted");
    const auto unrestricted = transition_id(*ids, "unrestricted");
    const auto pending_id = transition_id(*ids, "pending");
    const auto admitted = transition_id(*ids, "admitted");
    const auto establishing = transition_id(*ids, "establishing");
    const auto active_id = transition_id(*ids, "active");
    if (!proposed || !nominal || !restricted || !unrestricted || !pending_id ||
        !admitted || !establishing || !active_id) return false;
    if (!states.transition_lifecycle({*proposed, ids->connection,
            acs::LifecycleRevision{0}, acs::ConnectionLifecycle::proposed, 0}).ok() ||
        !states.transition_operational({*nominal, ids->connection,
            acs::OperationalRevision{0}, acs::OperationalCondition::nominal,
            states.retained_idempotency_horizon()}).ok()) return false;
    acs::EnforcementTransitionRequest enforcement{};
    enforcement.transition_id = *restricted;
    enforcement.connection_id = ids->connection;
    enforcement.desired = acs::EnforcementCondition::restricted;
    enforcement.subject = ids->source;
    enforcement.scope = {kScope};
    enforcement.authority = ids->authority;
    enforcement.known_idempotency_horizon = states.retained_idempotency_horizon();
    if (!states.transition_enforcement(enforcement).ok()) return false;
    enforcement.transition_id = *unrestricted;
    enforcement.expected_revision = acs::EnforcementRevision{1};
    enforcement.desired = acs::EnforcementCondition::unrestricted;
    enforcement.capability = ids->capability;
    enforcement.admission_reference = ids->evidence;
    if (!states.transition_enforcement(enforcement).ok() ||
        !states.transition_lifecycle({*pending_id, ids->connection,
            acs::LifecycleRevision{1}, acs::ConnectionLifecycle::admission_pending,
            states.retained_idempotency_horizon()}).ok()) return false;
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
    request.execution_policy_outcome = prometheus::core::AdmissionOutcome::accepted;
    request.budgets = {{{"messages"}, acs::BudgetUnit::messages,
        acs::BudgetUnit::messages, true, 1, 0, true, 16}};
    const auto admission_result = acs::evaluate_admission(snapshot, request);
    if (!admission_result.admitted() ||
        !states.transition_lifecycle({*admitted, ids->connection,
            acs::LifecycleRevision{2}, acs::ConnectionLifecycle::admitted,
            states.retained_idempotency_horizon()}).ok() ||
        !states.transition_lifecycle({*establishing, ids->connection,
            acs::LifecycleRevision{3}, acs::ConnectionLifecycle::establishing,
            states.retained_idempotency_horizon()}).ok() ||
        !states.transition_lifecycle({*active_id, ids->connection,
            acs::LifecycleRevision{4}, acs::ConnectionLifecycle::active,
            states.retained_idempotency_horizon()}).ok()) return false;
    const auto active = states.find(ids->connection);
    if (!active) return false;
    context.registry = snapshot;
    context.state = *active;
    context.admission = admission_result;
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
    context.envelope.correlation_id = ids->correlation;
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

std::optional<std::string> read_dmi(const char* name) {
    const std::string path = std::string{"/sys/class/dmi/id/"} + name;
    FILE* input = std::fopen(path.c_str(), "r");
    if (!input) return std::nullopt;
    std::array<char, kMaximumProfileBytes + 2> value{};
    const bool read = std::fgets(value.data(), static_cast<int>(value.size()), input);
    const bool at_end = read && std::fgetc(input) == EOF;
    const int closed = std::fclose(input);
    if (!read || !at_end || closed != 0) return std::nullopt;
    value[std::strcspn(value.data(), "\r\n")] = '\0';
    const std::string result{value.data()};
    if (result.empty() || result.size() > kMaximumProfileBytes) return std::nullopt;
    return result;
}

bool valid_node(const std::string& value) {
    if (value.empty() || value.size() > 32 || value.front() == '-' || value.back() == '-')
        return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-';
    });
}
bool valid_ipv4(const std::string& value) {
    in_addr parsed{};
    return inet_pton(AF_INET, value.c_str(), &parsed) == 1;
}

std::optional<Profile> read_profile() {
    const auto node = read_dmi("product_serial");
    const auto address = read_dmi("product_sku");
    const auto peers_a = read_dmi("sys_vendor");
    const auto peers_b = read_dmi("product_family");
    const auto version = read_dmi("product_version");
    constexpr const char* explicit_prefix = "acs-profile-v1-port-";
    constexpr const char* discovery_prefix = "acs-discovery-v1-port-";
    if (!node || !address || !peers_a || !peers_b || !version ||
        !valid_node(*node) || !valid_ipv4(*address)) return std::nullopt;
    AcsMode mode{};
    const char* port_text = nullptr;
    if (version->rfind(explicit_prefix, 0) == 0) {
        mode = AcsMode::explicit_peers;
        port_text = version->c_str() + std::strlen(explicit_prefix);
    } else if (version->rfind(discovery_prefix, 0) == 0 &&
               *peers_a == "none" && *peers_b == "none") {
        mode = AcsMode::discovery;
        port_text = version->c_str() + std::strlen(discovery_prefix);
    } else {
        return std::nullopt;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long port = std::strtoul(port_text, &end, 10);
    if (errno != 0 || !end || *end != '\0' || port < 1024 || port > 65535)
        return std::nullopt;
    Profile profile{*node, *address, static_cast<std::uint16_t>(port), {}, mode};
    if (mode == AcsMode::discovery) return profile;
    const std::string family = *peers_a + ";" + *peers_b;
    std::size_t begin = 0;
    while (begin < family.size()) {
        const std::size_t separator = family.find(';', begin);
        const std::size_t finish = separator == std::string::npos ? family.size() : separator;
        const std::string item = family.substr(begin, finish - begin);
        const std::size_t at = item.find('@');
        if (at == std::string::npos || item.find('@', at + 1) != std::string::npos)
            return std::nullopt;
        Peer peer{item.substr(0, at), item.substr(at + 1)};
        if (!valid_node(peer.node) || peer.node == profile.node ||
            !valid_ipv4(peer.address) || peer.address == profile.address ||
            (!profile.peers.empty() && profile.peers.back().node >= peer.node) ||
            profile.peers.size() >= kMaximumPeers) return std::nullopt;
        profile.peers.push_back(std::move(peer));
        if (separator == std::string::npos) break;
        begin = separator + 1;
    }
    if (profile.peers.empty()) return std::nullopt;
    return profile;
}

bool select_interface(int descriptor, std::array<char, IFNAMSIZ>& name) {
    ifreq flags{};
    std::snprintf(flags.ifr_name, sizeof(flags.ifr_name), "%s", "eth0");
    if (ioctl(descriptor, SIOCGIFFLAGS, &flags) != 0 ||
        (flags.ifr_flags & IFF_LOOPBACK) != 0) return false;
    std::snprintf(name.data(), name.size(), "%s", "eth0");
    return true;
}
bool configure_interface(int descriptor, const std::array<char, IFNAMSIZ>& name,
                         const std::string& address) {
    sockaddr_in parsed{};
    parsed.sin_family = AF_INET;
    if (inet_pton(AF_INET, address.c_str(), &parsed.sin_addr) != 1) return false;
    ifreq request{};
    std::snprintf(request.ifr_name, sizeof(request.ifr_name), "%s", name.data());
    std::memcpy(&request.ifr_addr, &parsed, sizeof(parsed));
    if (ioctl(descriptor, SIOCSIFADDR, &request) != 0) return false;
    parsed.sin_addr.s_addr = htonl(UINT32_C(0xffffff00));
    std::memcpy(&request.ifr_netmask, &parsed, sizeof(parsed));
    if (ioctl(descriptor, SIOCSIFNETMASK, &request) != 0 ||
        ioctl(descriptor, SIOCGIFFLAGS, &request) != 0) return false;
    request.ifr_flags = static_cast<short>(request.ifr_flags | IFF_UP);
    return ioctl(descriptor, SIOCSIFFLAGS, &request) == 0;
}

std::uint64_t monotonic_milliseconds() noexcept {
    timespec now{};
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) return 0;
    return static_cast<std::uint64_t>(now.tv_sec) * UINT64_C(1000) +
           static_cast<std::uint64_t>(now.tv_nsec) / UINT64_C(1000000);
}

std::optional<std::uint64_t> read_boot_epoch() noexcept {
    FILE* input = std::fopen("/proc/sys/kernel/random/boot_id", "r");
    if (!input) return std::nullopt;
    std::array<char, 64> value{};
    const bool read = std::fgets(value.data(), static_cast<int>(value.size()), input);
    const bool at_end = read && std::fgetc(input) == EOF;
    const int closed = std::fclose(input);
    if (!read || !at_end || closed != 0) return std::nullopt;
    value[std::strcspn(value.data(), "\r\n")] = '\0';
    const std::size_t length = std::strlen(value.data());
    if (length != 36) return std::nullopt;
    std::uint64_t hash = UINT64_C(1469598103934665603);
    for (std::size_t index = 0; index < length; ++index) {
        hash ^= static_cast<unsigned char>(value[index]);
        hash *= UINT64_C(1099511628211);
    }
    return hash == 0 ? UINT64_C(1) : hash;
}

struct DiscoveryStatistics {
    std::uint64_t packets_sent = 0;
    std::uint64_t packets_received = 0;
    std::uint64_t malformed_packets = 0;
    std::uint64_t evidence_events = 0;
};

acs::DiscoveryTransportBinding discovery_binding(
    const std::string& address) {
    return {{kDiscoveryTransportProfile}, address, kDiscoveryPort};
}

acs::DiscoveryMessage discovery_message(
    acs::DiscoveryMessageKind kind,
    const acs::ParticipantId& participant,
    const Profile& profile,
    std::uint64_t epoch,
    std::uint64_t nonce) {
    acs::DiscoveryMessage message{};
    message.kind = kind;
    message.participant = participant;
    message.binding = discovery_binding(profile.address);
    message.boot_epoch = epoch;
    message.nonce = nonce;
    return message;
}

bool send_discovery(
    int descriptor,
    const sockaddr_in& target,
    const acs::DiscoveryMessage& message,
    DiscoveryStatistics& statistics) noexcept {
    std::array<std::uint8_t, acs::kDiscoveryWireMaximumBytes> encoded{};
    const auto wire = acs::encode_discovery_message(
        message, encoded.data(), encoded.size());
    if (!wire.ok()) return false;
    const ssize_t sent = sendto(
        descriptor, encoded.data(), wire.bytes, 0,
        reinterpret_cast<const sockaddr*>(&target), sizeof(target));
    if (sent != static_cast<ssize_t>(wire.bytes)) return false;
    ++statistics.packets_sent;
    return true;
}

void emit_discovery_result(
    const Profile& profile,
    const acs::DiscoveryObservationResult& result,
    DiscoveryStatistics& statistics) noexcept {
    if (!result.evidence_required) return;
    ++statistics.evidence_events;
    const std::string peer = result.observation.participant.valid()
        ? result.observation.participant.value()
        : std::string{};
    switch (result.code) {
        case acs::DiscoveryObservationCode::observed:
        case acs::DiscoveryObservationCode::promoted_to_direct:
            emit("acs_participant_observed", profile.node, peer,
                 acs::to_string(result.code), "", "",
                 "direct_observation_only_relationship_candidate_no_authority");
            break;
        case acs::DiscoveryObservationCode::hinted:
            emit("acs_participant_hint_received", profile.node, peer,
                 "hinted", "", "",
                 "hint_only_not_direct_not_trusted_no_relationship");
            break;
        case acs::DiscoveryObservationCode::rediscovered:
            emit("acs_participant_rediscovered", profile.node, peer,
                 "observed", "", "",
                 "stale_to_observed_current_boot_transport_evidence");
            break;
        case acs::DiscoveryObservationCode::conflict:
        case acs::DiscoveryObservationCode::self_rejected:
            emit("acs_discovery_conflict", profile.node, peer,
                 acs::to_string(result.code), "", "",
                 "claim_conflicted_no_trust_or_authority_granted");
            break;
        case acs::DiscoveryObservationCode::capacity_exhausted:
            emit_local("acs_discovery_capacity_rejected", profile.node,
                       "capacity_exhausted",
                       "existing_observations_preserved");
            break;
        default:
            break;
    }
}

bool direct_transition(const acs::DiscoveryObservationResult& result) noexcept {
    return result.code == acs::DiscoveryObservationCode::observed ||
           result.code == acs::DiscoveryObservationCode::promoted_to_direct ||
           result.code == acs::DiscoveryObservationCode::rediscovered;
}

std::vector<acs::DiscoveryHint> discovery_hints(
    const acs::DiscoveryObservationStore& observations,
    const acs::ParticipantId& requester) {
    std::vector<acs::DiscoveryHint> hints;
    hints.reserve(acs::kDiscoveryWireMaximumHints);
    for (const auto& observation : observations.snapshot()) {
        if (hints.size() >= acs::kDiscoveryWireMaximumHints) break;
        if (observation.participant == requester ||
            observation.kind != acs::DiscoveryObservationKind::direct ||
            observation.state != acs::DiscoveryObservationState::observed) {
            continue;
        }
        hints.push_back({observation.participant, observation.binding,
                         observation.boot_epoch});
    }
    return hints;
}

int run_discovery(const Profile& profile) {
    const auto participant = acs::ParticipantId::parse("node", profile.node);
    const auto epoch = read_boot_epoch();
    if (!participant || !epoch) {
        emit_local("acs_discovery_ready", profile.node, "failed",
                   "local_identity_or_current_boot_epoch_unavailable");
        return 30;
    }
    acs::DiscoveryObservationStore observations{*participant};
    if (!observations.valid()) return 30;

    const int descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (descriptor < 0) {
        emit_local("acs_discovery_ready", profile.node, "unavailable",
                   "discovery_udp_socket_unavailable");
        return 0;
    }
    std::array<char, IFNAMSIZ> interface_name{};
    if (!select_interface(descriptor, interface_name) ||
        !configure_interface(descriptor, interface_name, profile.address)) {
        emit_local("acs_discovery_ready", profile.node, "unavailable",
                   "managed_interface_unavailable");
        (void)close(descriptor);
        return 0;
    }
    int enabled = 1;
    unsigned char ttl = 1;
    unsigned char loop = 0;
    in_addr local_address{};
    in_addr group_address{};
    if (inet_pton(AF_INET, profile.address.c_str(), &local_address) != 1 ||
        inet_pton(AF_INET, kDiscoveryGroup, &group_address) != 1 ||
        setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &enabled,
                   sizeof(enabled)) != 0 ||
        setsockopt(descriptor, IPPROTO_IP, IP_MULTICAST_IF, &local_address,
                   sizeof(local_address)) != 0 ||
        setsockopt(descriptor, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
                   sizeof(ttl)) != 0 ||
        setsockopt(descriptor, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
                   sizeof(loop)) != 0) {
        emit_local("acs_discovery_ready", profile.node, "failed",
                   "multicast_socket_configuration_failed");
        (void)close(descriptor);
        return 31;
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(kDiscoveryPort);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    ip_mreq membership{};
    membership.imr_multiaddr = group_address;
    membership.imr_interface = local_address;
    if (bind(descriptor, reinterpret_cast<const sockaddr*>(&local),
             sizeof(local)) != 0 ||
        setsockopt(descriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, &membership,
                   sizeof(membership)) != 0) {
        emit_local("acs_discovery_ready", profile.node, "failed",
                   "multicast_bind_or_membership_failed");
        (void)close(descriptor);
        return 31;
    }

    sockaddr_in multicast{};
    multicast.sin_family = AF_INET;
    multicast.sin_port = htons(kDiscoveryPort);
    multicast.sin_addr = group_address;
    int signal_descriptor = -1;
    if (lab_mode()) {
        sigset_t signals;
        (void)sigemptyset(&signals);
        (void)sigaddset(&signals, SIGTERM);
        (void)sigaddset(&signals, SIGINT);
        signal_descriptor = signalfd(
            -1, &signals, SFD_CLOEXEC | SFD_NONBLOCK);
        if (signal_descriptor < 0 || !write_mode_marker(AcsMode::discovery) ||
            !write_ready_marker()) {
            if (signal_descriptor >= 0) (void)close(signal_descriptor);
            (void)unlink(kResidentReadyPath);
            (void)unlink(kAcsModePath);
            (void)close(descriptor);
            return 32;
        }
    }

    emit_local("acs_discovery_started", profile.node, "active",
               "ipv4_local_multicast_239.77.0.1_port_39002");
    emit_local("acs_discovery_ready", profile.node, "ready",
               "local_transport_active_zero_remote_participants_valid");
    if (lab_mode()) {
        emit_local("acs_resident", profile.node, "active",
                   "public_discovery_transport_current_boot_lab_scope");
    }

    DiscoveryStatistics statistics{};
    const std::uint64_t started = monotonic_milliseconds();
    std::uint64_t next_announcement = 0;
    std::uint64_t nonce = *epoch;
    bool stopped = false;
    std::array<std::uint8_t, acs::kDiscoveryWireMaximumBytes> received{};
    while (!stopped) {
        const std::uint64_t now = monotonic_milliseconds();
        if (next_announcement == 0 || now >= next_announcement) {
            const auto presence = discovery_message(
                acs::DiscoveryMessageKind::presence, *participant,
                profile, *epoch, ++nonce);
            (void)send_discovery(descriptor, multicast, presence, statistics);
            next_announcement = now + kDiscoveryAnnouncementMilliseconds;
        }
        for (const auto& transition : observations.mark_stale(
                 now, kDiscoveryStaleMilliseconds)) {
            ++statistics.evidence_events;
            emit("acs_participant_stale", profile.node,
                 transition.participant.value(), "stale", "", "",
                 "absence_only_no_revocation_or_malice_inference");
        }
        if (!lab_mode() && now - started >= kDiscoveryConformanceMilliseconds) {
            break;
        }

        std::array<pollfd, 2> waits{{
            {signal_descriptor, POLLIN, 0},
            {descriptor, POLLIN, 0},
        }};
        const int polled = poll(waits.data(), waits.size(), kPollMilliseconds);
        if (polled < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (signal_descriptor >= 0 && (waits[0].revents & POLLIN) != 0) {
            signalfd_siginfo information{};
            if (read(signal_descriptor, &information, sizeof(information)) ==
                static_cast<ssize_t>(sizeof(information))) {
                stopped = true;
            }
        }
        if ((waits[1].revents & POLLIN) == 0) continue;

        sockaddr_in source{};
        iovec vector{received.data(), received.size()};
        msghdr packet{};
        packet.msg_name = &source;
        packet.msg_namelen = sizeof(source);
        packet.msg_iov = &vector;
        packet.msg_iovlen = 1;
        const ssize_t count = recvmsg(descriptor, &packet, MSG_TRUNC);
        if (count <= 0 || static_cast<std::size_t>(count) > received.size()) {
            ++statistics.malformed_packets;
            continue;
        }
        ++statistics.packets_received;
        acs::DiscoveryMessage message{};
        const auto decoded = acs::decode_discovery_message(
            received.data(), static_cast<std::size_t>(count), message);
        if (!decoded.ok()) {
            ++statistics.malformed_packets;
            continue;
        }
        std::array<char, INET_ADDRSTRLEN> source_text{};
        if (!inet_ntop(AF_INET, &source.sin_addr, source_text.data(),
                       source_text.size()) ||
            message.binding.address != source_text.data() ||
            message.binding.port != kDiscoveryPort ||
            message.binding.transport_profile.value != kDiscoveryTransportProfile) {
            ++statistics.evidence_events;
            emit("acs_discovery_conflict", profile.node,
                 message.participant.value(), "binding_mismatch", "", "",
                 "claimed_binding_did_not_match_observed_transport");
            continue;
        }

        const auto observed = observations.observe_direct(
            message.participant, message.binding, acs::kDiscoveryWireVersion,
            message.boot_epoch, now);
        emit_discovery_result(profile, observed, statistics);
        if (!observed.accepted()) continue;

        if (message.kind == acs::DiscoveryMessageKind::presence) {
            auto response = discovery_message(
                acs::DiscoveryMessageKind::response, *participant,
                profile, *epoch, message.nonce);
            (void)send_discovery(descriptor, source, response, statistics);
        }
        if (direct_transition(observed) &&
            (message.kind == acs::DiscoveryMessageKind::presence ||
             message.kind == acs::DiscoveryMessageKind::response)) {
            auto query = discovery_message(
                acs::DiscoveryMessageKind::hint_query, *participant,
                profile, *epoch, ++nonce);
            (void)send_discovery(descriptor, source, query, statistics);
        }
        if (message.kind == acs::DiscoveryMessageKind::hint_query) {
            auto response = discovery_message(
                acs::DiscoveryMessageKind::hint_response, *participant,
                profile, *epoch, message.nonce);
            response.hints = discovery_hints(observations, message.participant);
            (void)send_discovery(descriptor, source, response, statistics);
        } else if (message.kind == acs::DiscoveryMessageKind::hint_response) {
            for (const auto& hint : message.hints) {
                const auto result = observations.observe_hint(
                    hint.participant, hint.binding, acs::kDiscoveryWireVersion,
                    hint.boot_epoch, now);
                emit_discovery_result(profile, result, statistics);
            }
        }
    }

    char summary[256];
    (void)std::snprintf(
        summary, sizeof(summary),
        "sent=%llu,received=%llu,malformed=%llu,events=%llu,occupancy=%zu,peak=%zu",
        static_cast<unsigned long long>(statistics.packets_sent),
        static_cast<unsigned long long>(statistics.packets_received),
        static_cast<unsigned long long>(statistics.malformed_packets),
        static_cast<unsigned long long>(statistics.evidence_events),
        observations.size(), observations.peak_size());
    emit_local("acs_discovery_summary", profile.node, "bounded", summary);
    if (signal_descriptor >= 0) (void)close(signal_descriptor);
    (void)setsockopt(descriptor, IPPROTO_IP, IP_DROP_MEMBERSHIP, &membership,
                     sizeof(membership));
    (void)close(descriptor);
    (void)unlink(kResidentReadyPath);
    (void)unlink(kAcsModePath);
    if (lab_mode()) {
        emit_local("acs_resident", profile.node,
                   stopped ? "stopped" : "failed",
                   stopped ? "bounded_shutdown_complete"
                           : "resident_poll_failed");
    }
    return lab_mode() && !stopped ? 33 : 0;
}

bool wire_self_test(
    const std::array<std::uint8_t, acs::kReferenceWireMaximumBytes>& encoded,
    std::size_t size) {
    acs::SignalEnvelope ignored{};
    if (acs::decode_reference_signal(encoded.data(), 8, ignored).code !=
        acs::ReferenceWireCode::truncated) return false;
    auto malformed = encoded;
    malformed[0] = 0;
    if (acs::decode_reference_signal(malformed.data(), size, ignored).code !=
        acs::ReferenceWireCode::invalid_header) return false;
    malformed = encoded;
    malformed[5] = 2;
    if (acs::decode_reference_signal(malformed.data(), size, ignored).code !=
        acs::ReferenceWireCode::unsupported_version) return false;
    malformed = encoded;
    malformed[8] = 0; malformed[9] = 1; malformed[10] = 0; malformed[11] = 0;
    return acs::decode_reference_signal(malformed.data(), size, ignored).code ==
           acs::ReferenceWireCode::oversized;
}
std::string correlation(const acs::SignalEnvelope& envelope) {
    return envelope.correlation_id ? envelope.correlation_id->canonical() : "";
}

int run_transport(const Profile& profile) {
    std::vector<std::string> nodes{profile.node};
    for (const auto& peer : profile.peers) nodes.push_back(peer.node);
    std::sort(nodes.begin(), nodes.end());
    if (std::adjacent_find(nodes.begin(), nodes.end()) != nodes.end()) {
        emit_local("acs_transport_initialized", profile.node, "failed", "duplicate_profile_identity");
        return 20;
    }
    acs::AcsRegistry registry{};
    if (!populate_registry(registry, nodes)) {
        emit_local("acs_transport_initialized", profile.node, "failed",
                   "public_registry_initialization_failed");
        return 20;
    }
    acs::ConnectionStateStore states{registry};
    std::vector<PeerContext> contexts;
    contexts.reserve(profile.peers.size());
    for (const auto& peer : profile.peers) {
        PeerContext context{};
        context.peer = peer;
        if (!prepare_direction(registry, states, profile.node, peer.node, context.outgoing) ||
            !prepare_direction(registry, states, peer.node, profile.node, context.incoming)) {
            emit("acs_peer_contract", profile.node, peer.node, "failed", "", "",
                 "public_contract_initialization_failed");
            return 21;
        }
        const auto validation = acs::validate_transport_submission(
            context.outgoing.registry, context.outgoing.state, context.outgoing.admission,
            context.outgoing.binding, context.outgoing.attachment, context.outgoing.envelope);
        if (!validation.valid()) {
            emit("acs_signal_validation_result", profile.node, peer.node,
                 acs::to_string(validation.code), context.outgoing.envelope.id.canonical(),
                 correlation(context.outgoing.envelope), "sender_contract_validation_failed");
            return 22;
        }
        const auto wire = acs::encode_reference_signal(
            context.outgoing.envelope, context.encoded.data(), context.encoded.size());
        if (!wire.ok()) return 23;
        context.encoded_bytes = wire.bytes;
        context.endpoint.sin_family = AF_INET;
        context.endpoint.sin_port = htons(profile.port);
        if (inet_pton(AF_INET, peer.address.c_str(), &context.endpoint.sin_addr) != 1)
            return 23;
        emit("acs_transport_binding", profile.node, peer.node, "active",
             context.outgoing.envelope.id.canonical(), correlation(context.outgoing.envelope),
             "isolated.udp.ipv4.v1");
        emit("acs_transport_attachment", profile.node, peer.node, "active",
             context.outgoing.envelope.id.canonical(), correlation(context.outgoing.envelope),
             "peer_specific_public_conformance_attachment");
        contexts.push_back(std::move(context));
    }
    if (!wire_self_test(contexts.front().encoded, contexts.front().encoded_bytes)) return 24;
    emit_local("acs_wire_self_test", profile.node, "passed",
               "invalid_header_truncated_oversized_and_version_rejected");
    const int descriptor = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (descriptor < 0) {
        emit_local("acs_transport_initialized", profile.node, "unavailable", "udp_socket_unavailable");
        return 0;
    }
    std::array<char, IFNAMSIZ> interface_name{};
    if (!select_interface(descriptor, interface_name) ||
        !configure_interface(descriptor, interface_name, profile.address)) {
        emit_local("acs_transport_initialized", profile.node, "unavailable",
                   "managed_interface_unavailable");
        (void)close(descriptor);
        return 0;
    }
    sockaddr_in local{};
    local.sin_family = AF_INET;
    local.sin_port = htons(profile.port);
    if (inet_pton(AF_INET, profile.address.c_str(), &local.sin_addr) != 1 ||
        bind(descriptor, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) {
        emit_local("acs_transport_initialized", profile.node, "failed", "static_address_bind_failed");
        (void)close(descriptor);
        return 25;
    }
    emit_local("acs_network_configuration", profile.node, "configured", profile.address.c_str());
    emit_local("acs_transport_initialized", profile.node, "ready",
               "isolated_development_udp_not_production_secure");
    for (const auto& context : contexts) {
        emit("acs_signal_send_attempt", profile.node, context.peer.node, "attempted",
             context.outgoing.envelope.id.canonical(), correlation(context.outgoing.envelope),
             "bounded_public_conformance_signal");
    }
    bool malformed_sent = false;
    int grace = kGracePolls;
    std::array<std::uint8_t, acs::kReferenceWireMaximumBytes> received{};
    for (int attempt = 0; attempt < kMaximumPolls; ++attempt) {
        if (!malformed_sent && profile.node == nodes.front()) {
            auto malformed = contexts.front().encoded;
            malformed[0] = 0;
            (void)sendto(descriptor, malformed.data(), contexts.front().encoded_bytes, 0,
                reinterpret_cast<const sockaddr*>(&contexts.front().endpoint),
                sizeof(contexts.front().endpoint));
            malformed_sent = true;
            emit("acs_malformed_peer_test", profile.node, contexts.front().peer.node,
                 "transmitted", contexts.front().outgoing.envelope.id.canonical(),
                 correlation(contexts.front().outgoing.envelope),
                 "invalid_header_injected_before_valid_signal");
        }
        for (auto& context : contexts) {
            if (context.received) continue;
            const ssize_t count = sendto(descriptor, context.encoded.data(), context.encoded_bytes,
                0, reinterpret_cast<const sockaddr*>(&context.endpoint), sizeof(context.endpoint));
            if (count == static_cast<ssize_t>(context.encoded_bytes) && !context.sent) {
                context.sent = true;
                emit("acs_signal_send_result", profile.node, context.peer.node, "transmitted",
                     context.outgoing.envelope.id.canonical(), correlation(context.outgoing.envelope),
                     "udp_datagram_submitted_no_delivery_claim");
            }
        }
        pollfd wait{descriptor, POLLIN, 0};
        if (poll(&wait, 1, kPollMilliseconds) > 0 && (wait.revents & POLLIN) != 0) {
            sockaddr_in source{};
            iovec vector{received.data(), received.size()};
            msghdr message{};
            message.msg_name = &source;
            message.msg_namelen = sizeof(source);
            message.msg_iov = &vector;
            message.msg_iovlen = 1;
            const ssize_t count = recvmsg(descriptor, &message, MSG_TRUNC);
            if (count < 0) continue;
            auto context = std::find_if(contexts.begin(), contexts.end(),
                [&source](const PeerContext& item) {
                    return source.sin_addr.s_addr == item.endpoint.sin_addr.s_addr &&
                           source.sin_port == item.endpoint.sin_port;
                });
            if (context == contexts.end()) {
                emit_local("acs_signal_receive", profile.node, "rejected",
                           "unexpected_development_peer_endpoint");
                continue;
            }
            if (static_cast<std::size_t>(count) > received.size()) {
                emit("acs_signal_receive", profile.node, context->peer.node, "oversized", "", "",
                     "datagram_exceeded_reference_wire_bound");
                continue;
            }
            acs::SignalEnvelope decoded{};
            const auto decoded_result = acs::decode_reference_signal(
                received.data(), static_cast<std::size_t>(count), decoded);
            if (!decoded_result.ok()) {
                emit("acs_signal_receive", profile.node, context->peer.node,
                     acs::to_string(decoded_result.code), "", "",
                     "wire_validation_failed_peer_isolated");
                continue;
            }
            if (context->received) {
                if (!context->duplicate_reported) {
                    context->duplicate_reported = true;
                    emit("acs_signal_receive", profile.node, context->peer.node, "duplicate",
                         decoded.id.canonical(), correlation(decoded),
                         "bounded_retry_observed_and_suppressed");
                }
                continue;
            }
            emit("acs_signal_receive", profile.node, context->peer.node, "complete",
                 decoded.id.canonical(), correlation(decoded), "bounded_envelope_received");
            const auto validation = acs::validate_transport_submission(
                context->incoming.registry, context->incoming.state, context->incoming.admission,
                context->incoming.binding, context->incoming.attachment, decoded);
            emit("acs_signal_validation_result", profile.node, context->peer.node,
                 acs::to_string(validation.code), decoded.id.canonical(), correlation(decoded),
                 validation.valid() ? "public_contract_validation_complete"
                                    : "public_contract_validation_failed");
            if (validation.valid()) context->received = true;
        }
        const bool complete = std::all_of(contexts.begin(), contexts.end(),
            [](const PeerContext& item) { return item.received; });
        if (complete && --grace <= 0) break;
    }
    bool complete = true;
    for (const auto& context : contexts) {
        complete = complete && context.received;
        emit("acs_peer_result", profile.node, context.peer.node,
             context.received ? "complete" : "peer_unavailable",
             context.outgoing.envelope.id.canonical(), correlation(context.outgoing.envelope),
             context.received ? "peer_exchange_validated"
                              : "bounded_wait_expired_without_valid_signal");
    }
    if (complete && lab_mode()) {
        emit_local("acs_exchange_complete", profile.node, "complete",
                   "all_peer_exchanges_validated_before_residency");
        const int resident_result = remain_resident(profile, descriptor, contexts);
        (void)close(descriptor);
        return resident_result;
    }
    emit_local("acs_transport_closed", profile.node,
               complete ? "closed" : "peer_unavailable",
               complete ? "all_peer_exchanges_complete"
                        : "one_or_more_peer_exchanges_unavailable");
    (void)close(descriptor);
    return complete || !lab_mode() ? 0 : 29;
}

}  // namespace

int main() {
    if (std::getenv("NODE_P01_HOST_ROOT") != nullptr) {
        emit_local("acs_transport_initialized", "host-test", "unavailable",
                   "guest_networking_not_attempted_in_host_test");
        return 0;
    }
    const int serial = open("/dev/ttyS0", O_WRONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (serial >= 0) event_output_fd = serial;
    if (lab_mode()) {
        sigset_t signals;
        (void)sigemptyset(&signals);
        (void)sigaddset(&signals, SIGTERM);
        (void)sigaddset(&signals, SIGINT);
        if (sigprocmask(SIG_BLOCK, &signals, nullptr) != 0) {
            emit_local("acs_resident", "unmanaged-node", "failed",
                       "signal_mask_failed");
            return 27;
        }
    }
    const auto profile = read_profile();
    if (!profile) {
        emit_local("acs_transport_initialized", "unmanaged-node", "unavailable",
                   "managed_node_profile_not_present_or_malformed");
        return 0;
    }
    try {
        emit_local("acs_mode_selection", profile->node,
                   mode_name(profile->mode),
                   profile->mode == AcsMode::discovery
                       ? "explicit_discovery_profile_zero_remote_peers"
                       : "explicit_peer_conformance_profile");
        return profile->mode == AcsMode::discovery
            ? run_discovery(*profile)
            : run_transport(*profile);
    } catch (...) {
        emit_local("acs_transport_closed", profile->node, "resource_exhausted",
                   "exception_contained_at_service_boundary");
        return 26;
    }
}
