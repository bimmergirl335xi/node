#ifndef SRC_CORE_SERVICE_HPP
#define SRC_CORE_SERVICE_HPP

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace prometheus::core {

class ServiceContext;
class ServiceManager;

enum class ServiceState : std::uint8_t {
    constructed = 0,
    registered,
    initializing,
    initialized,
    starting,
    running,
    stopping,
    stopped,
    failed,
};

enum class ServiceCondition : std::uint8_t {
    unknown = 0,
    ready,
    degraded,
    unavailable,
    failed,
};

enum class ServicePhase : std::uint8_t {
    none = 0,
    registration,
    dependency_validation,
    initialization,
    startup,
    shutdown,
    observation,
};

enum class ServiceFailureCode : std::uint8_t {
    none = 0,
    invalid_state,
    rejected,
    exception,
};

struct ServiceFailure {
    ServiceFailureCode code = ServiceFailureCode::none;
    ServicePhase phase = ServicePhase::none;
    std::string service_id{};
    std::string message{};

    [[nodiscard]] bool present() const noexcept {
        return code != ServiceFailureCode::none;
    }
};

enum class ServiceOperationCode : std::uint8_t {
    success = 0,
    degraded,
    failed,
};

struct ServiceOperationResult {
    ServiceOperationCode code = ServiceOperationCode::success;
    std::string message{};

    [[nodiscard]] bool ok() const noexcept {
        return code == ServiceOperationCode::success ||
               code == ServiceOperationCode::degraded;
    }

    [[nodiscard]] static ServiceOperationResult degraded(std::string message);
    [[nodiscard]] static ServiceOperationResult failed(std::string message);
};

struct ServiceStatus {
    ServiceState state = ServiceState::constructed;
    ServiceCondition condition = ServiceCondition::unknown;
    bool usable = false;
    std::string message{};
    ServiceFailure last_failure{};
};

struct ServiceSnapshot {
    std::string service_id{};
    std::string display_name{};
    std::vector<std::string> dependencies{};
    ServiceStatus status{};
};

// Service supplies lifecycle-state handling and exception containment. Derived
// services implement only the three bounded hooks. The manager invokes these
// wrappers without holding its registry mutex.
class Service {
public:
    Service(std::string service_id,
            std::string display_name,
            std::vector<std::string> dependencies = {});
    virtual ~Service() = default;

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    Service(Service&&) = delete;
    Service& operator=(Service&&) = delete;

    [[nodiscard]] std::string_view service_id() const noexcept;
    [[nodiscard]] std::string_view display_name() const noexcept;
    [[nodiscard]] const std::vector<std::string>& dependencies() const noexcept;

    [[nodiscard]] ServiceStatus status() const;
    [[nodiscard]] ServiceSnapshot snapshot() const;

    [[nodiscard]] ServiceOperationResult initialize(
        ServiceContext& context) noexcept;
    [[nodiscard]] ServiceOperationResult start(ServiceContext& context) noexcept;
    [[nodiscard]] ServiceOperationResult shutdown(
        ServiceContext& context) noexcept;

protected:
    [[nodiscard]] virtual ServiceOperationResult on_initialize(
        ServiceContext& context) = 0;
    [[nodiscard]] virtual ServiceOperationResult on_start(
        ServiceContext& context) = 0;
    [[nodiscard]] virtual ServiceOperationResult on_shutdown(
        ServiceContext& context) = 0;

private:
    friend class ServiceManager;

    [[nodiscard]] bool mark_registered() noexcept;
    [[nodiscard]] ServiceOperationResult invoke_hook(
        ServicePhase phase,
        ServiceContext& context) noexcept;
    void record_result(ServicePhase phase,
                       const ServiceOperationResult& result) noexcept;
    [[nodiscard]] ServiceOperationResult invalid_state_result(
        ServicePhase phase,
        const char* operation) noexcept;

    std::string service_id_{};
    std::string display_name_{};
    std::vector<std::string> dependencies_{};
    mutable std::mutex mutex_{};
    ServiceStatus status_{};
};

[[nodiscard]] bool is_valid_service_id(std::string_view value) noexcept;
[[nodiscard]] const char* to_string(ServiceState value) noexcept;
[[nodiscard]] const char* to_string(ServiceCondition value) noexcept;
[[nodiscard]] const char* to_string(ServicePhase value) noexcept;
[[nodiscard]] const char* to_string(ServiceFailureCode value) noexcept;
[[nodiscard]] const char* to_string(ServiceOperationCode value) noexcept;

}  // namespace prometheus::core

#endif  // SRC_CORE_SERVICE_HPP
