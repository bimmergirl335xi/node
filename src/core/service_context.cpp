#include "service_context.hpp"

#include "backends/backend_registry.hpp"

namespace prometheus::core {

ServiceContext::ServiceContext(
    backends::BackendRegistry& backend_registry) noexcept
    : backend_registry_(&backend_registry) {}

backends::BackendRegistry& ServiceContext::backend_registry() const noexcept {
    return *backend_registry_;
}

}  // namespace prometheus::core
