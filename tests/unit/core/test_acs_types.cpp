#include <cstdlib>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "core/acs/acs_types.hpp"

namespace acs = prometheus::core::acs;

int main() {
    static_assert(!std::is_same_v<acs::ParticipantId, acs::AuthorityId>);
    const auto participant = acs::ParticipantId::parse("node", "participant-1");
    const auto authority = acs::AuthorityId::parse("authority", "runtime-1");
    if (!participant || !authority || participant->canonical() != "node:participant-1" ||
        acs::ParticipantId::parse("Node", "bad") ||
        acs::ParticipantId::parse("node", "bad value") ||
        acs::ParticipantId::parse("node", std::string(256, 'a'))) return EXIT_FAILURE;

    const acs::DescriptorRevision maximum{std::numeric_limits<std::uint64_t>::max()};
    std::uint64_t value = 0;
    if (maximum.next().has_value() ||
        acs::checked_add(std::numeric_limits<std::uint64_t>::max(), 1, value) ||
        acs::checked_multiply(std::numeric_limits<std::uint64_t>::max(), 2, value) ||
        !acs::checked_add(2, 3, value) || value != 5) return EXIT_FAILURE;

    acs::ScopeReference scope{"connection.control"};
    return scope.valid() ? EXIT_SUCCESS : EXIT_FAILURE;
}
