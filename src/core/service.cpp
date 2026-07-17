#include "service.hpp"

#include <cctype>
#include <exception>
#include <utility>

#include "service_context.hpp"

namespace prometheus::core {
namespace {

inline constexpr std::size_t kMaximumServiceIdLength = 128;

[[nodiscard]] ServiceState completed_state(ServicePhase phase) noexcept {
    switch (phase) {
        case ServicePhase::initialization:
            return ServiceState::initialized;
        case ServicePhase::startup:
            return ServiceState::running;
        case ServicePhase::shutdown:
            return ServiceState::stopped;
        default:
            return ServiceState::failed;
    }
}

}  // namespace

ServiceOperationResult ServiceOperationResult::degraded(std::string message) {
    return {ServiceOperationCode::degraded, std::move(message)};
}

ServiceOperationResult ServiceOperationResult::failed(std::string message) {
    return {ServiceOperationCode::failed, std::move(message)};
}

Service::Service(std::string service_id,
                 std::string display_name,
                 std::vector<std::string> dependencies)
    : service_id_(std::move(service_id)),
      display_name_(std::move(display_name)),
      dependencies_(std::move(dependencies)) {}

std::string_view Service::service_id() const noexcept {
    return service_id_;
}

std::string_view Service::display_name() const noexcept {
    return display_name_;
}

const std::vector<std::string>& Service::dependencies() const noexcept {
    return dependencies_;
}

ServiceStatus Service::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

ServiceSnapshot Service::snapshot() const {
    ServiceSnapshot result{};
    result.service_id = service_id_;
    result.display_name = display_name_;
    result.dependencies = dependencies_;
    result.status = status();
    return result;
}

bool Service::mark_registered() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (status_.state != ServiceState::constructed) {
        return false;
    }
    status_.state = ServiceState::registered;
    status_.condition = ServiceCondition::unknown;
    status_.usable = false;
    status_.message = "Service is registered";
    return true;
}

ServiceOperationResult Service::initialize(ServiceContext& context) noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_.state != ServiceState::registered) {
            return invalid_state_result(ServicePhase::initialization,
                                        "initialize");
        }
        status_.state = ServiceState::initializing;
        status_.condition = ServiceCondition::unknown;
        status_.usable = false;
        status_.message = "Service initialization is in progress";
    }
    return invoke_hook(ServicePhase::initialization, context);
}

ServiceOperationResult Service::start(ServiceContext& context) noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_.state != ServiceState::initialized) {
            return invalid_state_result(ServicePhase::startup, "start");
        }
        status_.state = ServiceState::starting;
        status_.condition = ServiceCondition::unknown;
        status_.usable = false;
        status_.message = "Service startup is in progress";
    }
    return invoke_hook(ServicePhase::startup, context);
}

ServiceOperationResult Service::shutdown(ServiceContext& context) noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (status_.state == ServiceState::stopped ||
            status_.state == ServiceState::constructed ||
            status_.state == ServiceState::registered) {
            return {};
        }
        if (status_.state == ServiceState::stopping) {
            return invalid_state_result(ServicePhase::shutdown, "shutdown");
        }
        status_.state = ServiceState::stopping;
        status_.condition = ServiceCondition::unavailable;
        status_.usable = false;
        status_.message = "Service shutdown is in progress";
    }
    return invoke_hook(ServicePhase::shutdown, context);
}

ServiceOperationResult Service::invoke_hook(ServicePhase phase,
                                            ServiceContext& context) noexcept {
    ServiceOperationResult result{};
    try {
        switch (phase) {
            case ServicePhase::initialization:
                result = on_initialize(context);
                break;
            case ServicePhase::startup:
                result = on_start(context);
                break;
            case ServicePhase::shutdown:
                result = on_shutdown(context);
                break;
            default:
                result = ServiceOperationResult::failed(
                    "Unsupported service lifecycle phase");
                break;
        }
    } catch (const std::exception& error) {
        result = ServiceOperationResult::failed(
            std::string{"Service lifecycle callback threw an exception: "} +
            error.what());
        std::lock_guard<std::mutex> lock(mutex_);
        status_.last_failure.code = ServiceFailureCode::exception;
    } catch (...) {
        result = ServiceOperationResult::failed(
            "Service lifecycle callback threw an unknown exception");
        std::lock_guard<std::mutex> lock(mutex_);
        status_.last_failure.code = ServiceFailureCode::exception;
    }

    record_result(phase, result);
    return result;
}

void Service::record_result(ServicePhase phase,
                            const ServiceOperationResult& result) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (result.ok()) {
        status_.state = completed_state(phase);
        status_.condition = result.code == ServiceOperationCode::degraded
                                ? ServiceCondition::degraded
                                : (phase == ServicePhase::shutdown
                                       ? ServiceCondition::unavailable
                                       : ServiceCondition::ready);
        status_.usable = phase != ServicePhase::shutdown;
        status_.message = result.message;
        return;
    }

    status_.state = ServiceState::failed;
    status_.condition = ServiceCondition::failed;
    status_.usable = false;
    status_.message = result.message;
    if (status_.last_failure.code != ServiceFailureCode::exception) {
        status_.last_failure.code = ServiceFailureCode::rejected;
    }
    status_.last_failure.phase = phase;
    status_.last_failure.service_id = service_id_;
    status_.last_failure.message = result.message;
}

ServiceOperationResult Service::invalid_state_result(
    ServicePhase phase,
    const char* operation) noexcept {
    const std::string message = std::string{"Cannot "} + operation +
                                " service '" + service_id_ +
                                "' from state " + to_string(status_.state);
    status_.last_failure = {ServiceFailureCode::invalid_state,
                            phase,
                            service_id_,
                            message};
    return ServiceOperationResult::failed(message);
}

bool is_valid_service_id(std::string_view value) noexcept {
    if (value.empty() || value.size() > kMaximumServiceIdLength ||
        !std::isalnum(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    for (const char character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (!std::isalnum(byte) && character != '.' && character != '_' &&
            character != '-') {
            return false;
        }
    }
    return true;
}

const char* to_string(ServiceState value) noexcept {
    switch (value) {
        case ServiceState::constructed: return "constructed";
        case ServiceState::registered: return "registered";
        case ServiceState::initializing: return "initializing";
        case ServiceState::initialized: return "initialized";
        case ServiceState::starting: return "starting";
        case ServiceState::running: return "running";
        case ServiceState::stopping: return "stopping";
        case ServiceState::stopped: return "stopped";
        case ServiceState::failed: return "failed";
    }
    return "unknown";
}

const char* to_string(ServiceCondition value) noexcept {
    switch (value) {
        case ServiceCondition::unknown: return "unknown";
        case ServiceCondition::ready: return "ready";
        case ServiceCondition::degraded: return "degraded";
        case ServiceCondition::unavailable: return "unavailable";
        case ServiceCondition::failed: return "failed";
    }
    return "unknown";
}

const char* to_string(ServicePhase value) noexcept {
    switch (value) {
        case ServicePhase::none: return "none";
        case ServicePhase::registration: return "registration";
        case ServicePhase::dependency_validation: return "dependency_validation";
        case ServicePhase::initialization: return "initialization";
        case ServicePhase::startup: return "startup";
        case ServicePhase::shutdown: return "shutdown";
        case ServicePhase::observation: return "observation";
    }
    return "unknown";
}

const char* to_string(ServiceFailureCode value) noexcept {
    switch (value) {
        case ServiceFailureCode::none: return "none";
        case ServiceFailureCode::invalid_state: return "invalid_state";
        case ServiceFailureCode::rejected: return "rejected";
        case ServiceFailureCode::exception: return "exception";
    }
    return "unknown";
}

const char* to_string(ServiceOperationCode value) noexcept {
    switch (value) {
        case ServiceOperationCode::success: return "success";
        case ServiceOperationCode::degraded: return "degraded";
        case ServiceOperationCode::failed: return "failed";
    }
    return "unknown";
}

}  // namespace prometheus::core
