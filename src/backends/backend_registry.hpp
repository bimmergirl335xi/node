#ifndef SRC_BACKENDS_BACKEND_REGISTRY_HPP
#define SRC_BACKENDS_BACKEND_REGISTRY_HPP

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "compute_backend.hpp"

namespace prometheus::backends {

enum class BackendRegistrationCode : std::uint8_t {
    success = 0,
    null_backend,
    invalid_backend_id,
    duplicate_backend_id,
    not_found,
};

struct BackendRegistrationResult {
    BackendRegistrationCode code = BackendRegistrationCode::success;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == BackendRegistrationCode::success;
    }
};

// Registry ownership is explicit and local to a node runtime. Registration does
// not initialize a backend, select hardware, or dispatch work.
class BackendRegistry {
public:
    [[nodiscard]] BackendRegistrationResult register_backend(
        std::shared_ptr<ComputeBackend> backend);

    [[nodiscard]] BackendRegistrationResult remove_backend(
        std::string_view backend_id);

    [[nodiscard]] std::shared_ptr<ComputeBackend> find_backend(
        std::string_view backend_id) const;

    [[nodiscard]] std::vector<ComputeBackendSnapshot> snapshots() const;
    [[nodiscard]] std::size_t size() const;

private:
    mutable std::mutex mutex_{};
    std::vector<std::shared_ptr<ComputeBackend>> backends_{};
};

[[nodiscard]] const char* to_string(BackendRegistrationCode value) noexcept;

}  // namespace prometheus::backends

#endif  // SRC_BACKENDS_BACKEND_REGISTRY_HPP
