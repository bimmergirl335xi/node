#ifndef SRC_CORE_SERVICE_MANAGER_HPP
#define SRC_CORE_SERVICE_MANAGER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "service.hpp"

namespace prometheus::core {

class ServiceContext;

enum class ServiceManagerCode : std::uint8_t {
    success = 0,
    null_service,
    invalid_service_id,
    invalid_dependency_id,
    duplicate_service_id,
    service_state_conflict,
    lifecycle_busy,
    missing_dependency,
    self_dependency,
    dependency_cycle,
    initialization_failed,
    startup_failed,
    shutdown_failed,
};

struct ServiceManagerResult {
    ServiceManagerCode code = ServiceManagerCode::success;
    std::string service_id{};
    std::string message{};
    std::vector<std::string> lifecycle_order{};
    std::vector<ServiceFailure> failures{};

    [[nodiscard]] bool ok() const noexcept {
        return code == ServiceManagerCode::success;
    }
};

// ServiceManager owns registered services through shared ownership. It borrows
// a ServiceContext, which must outlive the manager and every lifecycle call.
class ServiceManager {
public:
    explicit ServiceManager(ServiceContext& context) noexcept;

    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;
    ServiceManager(ServiceManager&&) = delete;
    ServiceManager& operator=(ServiceManager&&) = delete;

    [[nodiscard]] ServiceManagerResult register_service(
        std::shared_ptr<Service> service);

    [[nodiscard]] std::shared_ptr<Service> find_service(
        std::string_view service_id) const;
    [[nodiscard]] std::vector<ServiceSnapshot> snapshots() const;
    [[nodiscard]] std::size_t size() const;

    [[nodiscard]] ServiceManagerResult validate_dependencies() const;
    [[nodiscard]] ServiceManagerResult initialize_all();
    [[nodiscard]] ServiceManagerResult start_all();
    [[nodiscard]] ServiceManagerResult shutdown_all() noexcept;

private:
    struct RegisteredService {
        std::string service_id{};
        std::shared_ptr<Service> service{};
    };

    [[nodiscard]] ServiceManagerResult dependency_order_locked() const;
    [[nodiscard]] bool begin_lifecycle_locked(
        ServiceManagerResult& result) noexcept;
    void end_lifecycle() noexcept;
    [[nodiscard]] std::vector<std::shared_ptr<Service>> services_for_order(
        const std::vector<std::string>& order) const;
    void rollback_services(
        const std::vector<std::shared_ptr<Service>>& services,
        ServiceManagerResult& result) noexcept;

    ServiceContext* context_ = nullptr;
    mutable std::mutex mutex_{};
    std::vector<RegisteredService> services_{};
    std::vector<std::string> initialized_order_{};
    bool lifecycle_in_progress_ = false;
};

[[nodiscard]] const char* to_string(ServiceManagerCode value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_SERVICE_MANAGER_HPP
