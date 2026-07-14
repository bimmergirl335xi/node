#include "backend_registry.hpp"

#include <algorithm>
#include <utility>

namespace prometheus::backends {

BackendRegistrationResult BackendRegistry::register_backend(
    std::shared_ptr<ComputeBackend> backend) {
    if (!backend) {
        return {BackendRegistrationCode::null_backend,
                "Cannot register a null compute backend"};
    }

    const std::string id{backend->backend_id()};
    if (id.empty()) {
        return {BackendRegistrationCode::invalid_backend_id,
                "Compute backend ID cannot be empty"};
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const auto duplicate = std::find_if(
        backends_.begin(),
        backends_.end(),
        [&id](const std::shared_ptr<ComputeBackend>& existing) {
            return existing && existing->backend_id() == id;
        });
    if (duplicate != backends_.end()) {
        return {BackendRegistrationCode::duplicate_backend_id,
                "A compute backend with ID '" + id + "' is already registered"};
    }

    backends_.push_back(std::move(backend));
    return {};
}

BackendRegistrationResult BackendRegistry::remove_backend(
    std::string_view backend_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(
        backends_.begin(),
        backends_.end(),
        [backend_id](const std::shared_ptr<ComputeBackend>& backend) {
            return backend && backend->backend_id() == backend_id;
        });
    if (iterator == backends_.end()) {
        return {BackendRegistrationCode::not_found,
                "Compute backend was not found"};
    }

    backends_.erase(iterator);
    return {};
}

std::shared_ptr<ComputeBackend> BackendRegistry::find_backend(
    std::string_view backend_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(
        backends_.begin(),
        backends_.end(),
        [backend_id](const std::shared_ptr<ComputeBackend>& backend) {
            return backend && backend->backend_id() == backend_id;
        });
    return iterator == backends_.end() ? nullptr : *iterator;
}

std::vector<ComputeBackendSnapshot> BackendRegistry::snapshots() const {
    std::vector<std::shared_ptr<ComputeBackend>> copy{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        copy = backends_;
    }

    std::vector<ComputeBackendSnapshot> result{};
    result.reserve(copy.size());
    for (const std::shared_ptr<ComputeBackend>& backend : copy) {
        if (backend) {
            result.push_back(backend->snapshot());
        }
    }
    return result;
}

std::size_t BackendRegistry::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backends_.size();
}

const char* to_string(BackendRegistrationCode value) noexcept {
    switch (value) {
        case BackendRegistrationCode::success:
            return "success";
        case BackendRegistrationCode::null_backend:
            return "null_backend";
        case BackendRegistrationCode::invalid_backend_id:
            return "invalid_backend_id";
        case BackendRegistrationCode::duplicate_backend_id:
            return "duplicate_backend_id";
        case BackendRegistrationCode::not_found:
            return "not_found";
    }
    return "unknown";
}

}  // namespace prometheus::backends
