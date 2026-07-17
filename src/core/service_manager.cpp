#include "service_manager.hpp"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>

#include "service_context.hpp"

namespace prometheus::core {
namespace {

[[nodiscard]] ServiceFailure make_failure(
    const Service& service,
    ServicePhase phase,
    const ServiceOperationResult& result) {
    ServiceFailure failure = service.status().last_failure;
    if (!failure.present()) {
        failure.code = ServiceFailureCode::rejected;
        failure.phase = phase;
        failure.service_id = std::string{service.service_id()};
        failure.message = result.message;
    }
    return failure;
}

}  // namespace

ServiceManager::ServiceManager(ServiceContext& context) noexcept
    : context_(&context) {}

ServiceManagerResult ServiceManager::register_service(
    std::shared_ptr<Service> service) {
    if (!service) {
        return {ServiceManagerCode::null_service,
                {},
                "Cannot register a null service"};
    }

    const std::string service_id{service->service_id()};
    if (!is_valid_service_id(service_id)) {
        return {ServiceManagerCode::invalid_service_id,
                service_id,
                "Service ID is empty, too long, or contains invalid characters"};
    }
    for (const std::string& dependency : service->dependencies()) {
        if (!is_valid_service_id(dependency)) {
            return {ServiceManagerCode::invalid_dependency_id,
                    service_id,
                    "Service dependency ID is invalid: '" + dependency + "'"};
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (lifecycle_in_progress_) {
        return {ServiceManagerCode::lifecycle_busy,
                service_id,
                "Cannot register a service during a lifecycle operation"};
    }
    const auto duplicate = std::find_if(
        services_.begin(),
        services_.end(),
        [&service_id](const RegisteredService& existing) {
            return existing.service_id == service_id;
        });
    if (duplicate != services_.end()) {
        return {ServiceManagerCode::duplicate_service_id,
                service_id,
                "A service with ID '" + service_id + "' is already registered"};
    }
    if (!service->mark_registered()) {
        return {ServiceManagerCode::service_state_conflict,
                service_id,
                "Only a constructed service may be registered"};
    }

    services_.push_back({service_id, std::move(service)});
    return {};
}

std::shared_ptr<Service> ServiceManager::find_service(
    std::string_view service_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto iterator = std::find_if(
        services_.begin(),
        services_.end(),
        [service_id](const RegisteredService& registered) {
            return registered.service_id == service_id;
        });
    return iterator == services_.end() ? nullptr : iterator->service;
}

std::vector<ServiceSnapshot> ServiceManager::snapshots() const {
    std::vector<std::shared_ptr<Service>> services{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        services.reserve(services_.size());
        for (const RegisteredService& registered : services_) {
            services.push_back(registered.service);
        }
    }

    std::vector<ServiceSnapshot> result{};
    result.reserve(services.size());
    for (const std::shared_ptr<Service>& service : services) {
        result.push_back(service->snapshot());
    }
    std::sort(result.begin(), result.end(),
              [](const ServiceSnapshot& left, const ServiceSnapshot& right) {
                  return left.service_id < right.service_id;
              });
    return result;
}

std::size_t ServiceManager::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_.size();
}

ServiceManagerResult ServiceManager::validate_dependencies() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dependency_order_locked();
}

ServiceManagerResult ServiceManager::initialize_all() {
    ServiceManagerResult result{};
    std::vector<std::string> order{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!begin_lifecycle_locked(result)) {
            return result;
        }
        result = dependency_order_locked();
        if (!result.ok()) {
            lifecycle_in_progress_ = false;
            return result;
        }
        order = result.lifecycle_order;
    }

    const std::vector<std::shared_ptr<Service>> services =
        services_for_order(order);
    std::vector<std::shared_ptr<Service>> initialized{};
    initialized.reserve(services.size());
    for (const std::shared_ptr<Service>& service : services) {
        const ServiceOperationResult operation = service->initialize(*context_);
        if (!operation.ok()) {
            result.code = ServiceManagerCode::initialization_failed;
            result.service_id = std::string{service->service_id()};
            result.message = operation.message;
            result.failures.push_back(make_failure(
                *service, ServicePhase::initialization, operation));
            initialized.push_back(service);
            rollback_services(initialized, result);
            end_lifecycle();
            return result;
        }
        initialized.push_back(service);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_order_ = order;
        lifecycle_in_progress_ = false;
    }
    return result;
}

ServiceManagerResult ServiceManager::start_all() {
    ServiceManagerResult result{};
    std::vector<std::string> order{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!begin_lifecycle_locked(result)) {
            return result;
        }
        if (initialized_order_.empty() && !services_.empty()) {
            lifecycle_in_progress_ = false;
            return {ServiceManagerCode::service_state_conflict,
                    {},
                    "Services must be initialized before startup"};
        }
        order = initialized_order_;
        result.lifecycle_order = order;
    }

    const std::vector<std::shared_ptr<Service>> services =
        services_for_order(order);
    for (const std::shared_ptr<Service>& service : services) {
        const ServiceOperationResult operation = service->start(*context_);
        if (!operation.ok()) {
            result.code = ServiceManagerCode::startup_failed;
            result.service_id = std::string{service->service_id()};
            result.message = operation.message;
            result.failures.push_back(
                make_failure(*service, ServicePhase::startup, operation));
            rollback_services(services, result);
            end_lifecycle();
            return result;
        }
    }

    end_lifecycle();
    return result;
}

ServiceManagerResult ServiceManager::shutdown_all() noexcept {
    ServiceManagerResult result{};
    std::vector<std::string> order{};
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!begin_lifecycle_locked(result)) {
            return result;
        }
        order = initialized_order_;
        result.lifecycle_order = order;
    }

    std::vector<std::shared_ptr<Service>> services = services_for_order(order);
    rollback_services(services, result);
    end_lifecycle();
    if (!result.failures.empty()) {
        result.code = ServiceManagerCode::shutdown_failed;
        result.message = "One or more services failed during shutdown";
    }
    return result;
}

ServiceManagerResult ServiceManager::dependency_order_locked() const {
    ServiceManagerResult result{};
    std::unordered_map<std::string, const RegisteredService*> by_id{};
    by_id.reserve(services_.size());
    for (const RegisteredService& service : services_) {
        by_id.emplace(service.service_id, &service);
    }

    for (const RegisteredService& service : services_) {
        for (const std::string& dependency : service.service->dependencies()) {
            if (dependency == service.service_id) {
                return {ServiceManagerCode::self_dependency,
                        service.service_id,
                        "Service '" + service.service_id +
                            "' depends on itself"};
            }
            if (by_id.find(dependency) == by_id.end()) {
                return {ServiceManagerCode::missing_dependency,
                        service.service_id,
                        "Service '" + service.service_id +
                            "' requires missing dependency '" + dependency +
                            "'"};
            }
        }
    }

    std::vector<std::string> ids{};
    ids.reserve(services_.size());
    for (const RegisteredService& service : services_) {
        ids.push_back(service.service_id);
    }
    std::sort(ids.begin(), ids.end());

    enum class Visit : std::uint8_t { unvisited = 0, visiting, visited };
    std::unordered_map<std::string, Visit> visits{};
    visits.reserve(ids.size());
    for (const std::string& id : ids) {
        visits.emplace(id, Visit::unvisited);
    }

    std::function<bool(const std::string&)> visit =
        [&](const std::string& id) {
            Visit& state = visits.at(id);
            if (state == Visit::visiting) {
                result.code = ServiceManagerCode::dependency_cycle;
                result.service_id = id;
                result.message = "Service dependency graph contains a cycle";
                return false;
            }
            if (state == Visit::visited) {
                return true;
            }
            state = Visit::visiting;
            std::vector<std::string> dependencies =
                by_id.at(id)->service->dependencies();
            std::sort(dependencies.begin(), dependencies.end());
            dependencies.erase(
                std::unique(dependencies.begin(), dependencies.end()),
                dependencies.end());
            for (const std::string& dependency : dependencies) {
                if (!visit(dependency)) {
                    return false;
                }
            }
            state = Visit::visited;
            result.lifecycle_order.push_back(id);
            return true;
        };

    for (const std::string& id : ids) {
        if (!visit(id)) {
            result.lifecycle_order.clear();
            return result;
        }
    }
    return result;
}

bool ServiceManager::begin_lifecycle_locked(
    ServiceManagerResult& result) noexcept {
    if (lifecycle_in_progress_) {
        result.code = ServiceManagerCode::lifecycle_busy;
        result.message = "A service lifecycle operation is already in progress";
        return false;
    }
    lifecycle_in_progress_ = true;
    return true;
}

void ServiceManager::end_lifecycle() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    lifecycle_in_progress_ = false;
}

std::vector<std::shared_ptr<Service>> ServiceManager::services_for_order(
    const std::vector<std::string>& order) const {
    std::vector<std::shared_ptr<Service>> result{};
    result.reserve(order.size());
    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::string& id : order) {
        const auto iterator = std::find_if(
            services_.begin(),
            services_.end(),
            [&id](const RegisteredService& service) {
                return service.service_id == id;
            });
        if (iterator != services_.end()) {
            result.push_back(iterator->service);
        }
    }
    return result;
}

void ServiceManager::rollback_services(
    const std::vector<std::shared_ptr<Service>>& services,
    ServiceManagerResult& result) noexcept {
    for (auto iterator = services.rbegin(); iterator != services.rend();
         ++iterator) {
        const ServiceOperationResult operation = (*iterator)->shutdown(*context_);
        if (!operation.ok()) {
            result.failures.push_back(make_failure(
                **iterator, ServicePhase::shutdown, operation));
        }
    }
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_order_.clear();
}

const char* to_string(ServiceManagerCode value) noexcept {
    switch (value) {
        case ServiceManagerCode::success: return "success";
        case ServiceManagerCode::null_service: return "null_service";
        case ServiceManagerCode::invalid_service_id: return "invalid_service_id";
        case ServiceManagerCode::invalid_dependency_id: return "invalid_dependency_id";
        case ServiceManagerCode::duplicate_service_id: return "duplicate_service_id";
        case ServiceManagerCode::service_state_conflict: return "service_state_conflict";
        case ServiceManagerCode::lifecycle_busy: return "lifecycle_busy";
        case ServiceManagerCode::missing_dependency: return "missing_dependency";
        case ServiceManagerCode::self_dependency: return "self_dependency";
        case ServiceManagerCode::dependency_cycle: return "dependency_cycle";
        case ServiceManagerCode::initialization_failed: return "initialization_failed";
        case ServiceManagerCode::startup_failed: return "startup_failed";
        case ServiceManagerCode::shutdown_failed: return "shutdown_failed";
    }
    return "unknown";
}

}  // namespace prometheus::core
