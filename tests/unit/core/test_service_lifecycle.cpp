#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "backends/backend_registry.hpp"
#include "core/service.hpp"
#include "core/service_context.hpp"
#include "core/service_manager.hpp"

namespace pb = prometheus::backends;
namespace pc = prometheus::core;

namespace {

struct Behavior {
    bool fail_initialize = false;
    bool fail_start = false;
    bool fail_shutdown = false;
    bool throw_initialize = false;
    bool throw_start = false;
    bool throw_shutdown = false;
};

class RecordingService final : public pc::Service {
public:
    RecordingService(std::string id,
                     std::vector<std::string> dependencies,
                     std::vector<std::string>& events,
                     Behavior behavior = {})
        : Service(std::move(id), "Recording service", std::move(dependencies)),
          events_(events),
          behavior_(behavior) {}

    [[nodiscard]] bool saw_backend_registry() const noexcept {
        return saw_backend_registry_;
    }

protected:
    pc::ServiceOperationResult on_initialize(
        pc::ServiceContext& context) override {
        (void)context.backend_registry().size();
        saw_backend_registry_ = true;
        events_.push_back("initialize:" + std::string{service_id()});
        if (behavior_.throw_initialize) {
            throw std::runtime_error("synthetic initialization exception");
        }
        if (behavior_.fail_initialize) {
            return pc::ServiceOperationResult::failed(
                "synthetic initialization failure");
        }
        return {};
    }

    pc::ServiceOperationResult on_start(pc::ServiceContext&) override {
        events_.push_back("start:" + std::string{service_id()});
        if (behavior_.throw_start) {
            throw std::runtime_error("synthetic startup exception");
        }
        if (behavior_.fail_start) {
            return pc::ServiceOperationResult::failed(
                "synthetic startup failure");
        }
        return {};
    }

    pc::ServiceOperationResult on_shutdown(pc::ServiceContext&) override {
        events_.push_back("shutdown:" + std::string{service_id()});
        if (behavior_.throw_shutdown) {
            throw std::runtime_error("synthetic shutdown exception");
        }
        if (behavior_.fail_shutdown) {
            return pc::ServiceOperationResult::failed(
                "synthetic shutdown failure");
        }
        return {};
    }

private:
    std::vector<std::string>& events_;
    Behavior behavior_{};
    bool saw_backend_registry_ = false;
};

[[nodiscard]] bool equals(const std::vector<std::string>& actual,
                          std::initializer_list<const char*> expected) {
    if (actual.size() != expected.size()) {
        return false;
    }
    std::size_t index = 0;
    for (const char* value : expected) {
        if (actual[index++] != value) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool test_registration_and_dependencies() {
    pb::BackendRegistry registry{};
    pc::ServiceContext context{registry};
    pc::ServiceManager manager{context};
    std::vector<std::string> events{};

    if (&context.backend_registry() != &registry || manager.size() != 0 ||
        !manager.snapshots().empty() ||
        !manager.validate_dependencies().ok() ||
        !manager.initialize_all().ok() || !manager.start_all().ok() ||
        !manager.shutdown_all().ok()) {
        return false;
    }
    if (manager.register_service(nullptr).code !=
        pc::ServiceManagerCode::null_service) {
        return false;
    }
    auto invalid = std::make_shared<RecordingService>(
        "bad id", std::vector<std::string>{}, events);
    if (manager.register_service(invalid).code !=
        pc::ServiceManagerCode::invalid_service_id) {
        return false;
    }

    auto alpha = std::make_shared<RecordingService>(
        "alpha", std::vector<std::string>{}, events);
    if (!manager.register_service(alpha).ok() || manager.size() != 1 ||
        manager.find_service("alpha") != alpha ||
        manager.find_service("missing") != nullptr ||
        manager.register_service(alpha).code !=
            pc::ServiceManagerCode::duplicate_service_id) {
        return false;
    }
    const pc::ServiceSnapshot registered = manager.snapshots().front();
    return registered.service_id == "alpha" &&
           registered.status.state == pc::ServiceState::registered &&
           events.empty();
}

[[nodiscard]] bool test_dependency_errors() {
    pb::BackendRegistry registry{};
    pc::ServiceContext context{registry};
    std::vector<std::string> events{};

    pc::ServiceManager missing{context};
    if (!missing.register_service(std::make_shared<RecordingService>(
             "consumer", std::vector<std::string>{"provider"}, events)).ok() ||
        missing.validate_dependencies().code !=
            pc::ServiceManagerCode::missing_dependency) {
        return false;
    }

    pc::ServiceManager self{context};
    if (!self.register_service(std::make_shared<RecordingService>(
             "self", std::vector<std::string>{"self"}, events)).ok() ||
        self.validate_dependencies().code !=
            pc::ServiceManagerCode::self_dependency) {
        return false;
    }

    pc::ServiceManager cycle{context};
    if (!cycle.register_service(std::make_shared<RecordingService>(
             "a", std::vector<std::string>{"b"}, events)).ok() ||
        !cycle.register_service(std::make_shared<RecordingService>(
             "b", std::vector<std::string>{"c"}, events)).ok() ||
        !cycle.register_service(std::make_shared<RecordingService>(
             "c", std::vector<std::string>{"a"}, events)).ok() ||
        cycle.validate_dependencies().code !=
            pc::ServiceManagerCode::dependency_cycle) {
        return false;
    }
    return true;
}

[[nodiscard]] bool test_order_and_context() {
    pb::BackendRegistry registry{};
    pc::ServiceContext context{registry};
    pc::ServiceManager manager{context};
    std::vector<std::string> events{};

    auto leaf = std::make_shared<RecordingService>(
        "leaf", std::vector<std::string>{"middle", "side"}, events);
    auto side = std::make_shared<RecordingService>(
        "side", std::vector<std::string>{"root"}, events);
    auto root = std::make_shared<RecordingService>(
        "root", std::vector<std::string>{}, events);
    auto middle = std::make_shared<RecordingService>(
        "middle", std::vector<std::string>{"root"}, events);
    if (!manager.register_service(leaf).ok() ||
        !manager.register_service(side).ok() ||
        !manager.register_service(root).ok() ||
        !manager.register_service(middle).ok()) {
        return false;
    }
    const pc::ServiceManagerResult order = manager.validate_dependencies();
    if (!order.ok() || order.lifecycle_order !=
            std::vector<std::string>({"root", "middle", "side", "leaf"})) {
        return false;
    }
    if (!manager.initialize_all().ok() || !equals(events, {
            "initialize:root", "initialize:middle", "initialize:side",
            "initialize:leaf"})) {
        return false;
    }
    if (!root->saw_backend_registry() || !middle->saw_backend_registry() ||
        !side->saw_backend_registry() || !leaf->saw_backend_registry()) {
        return false;
    }
    if (!manager.start_all().ok() || !equals(events, {
            "initialize:root", "initialize:middle", "initialize:side",
            "initialize:leaf", "start:root", "start:middle", "start:side",
            "start:leaf"})) {
        return false;
    }
    for (const pc::ServiceSnapshot& snapshot : manager.snapshots()) {
        if (snapshot.status.state != pc::ServiceState::running ||
            !snapshot.status.usable || snapshot.display_name.empty()) {
            return false;
        }
    }
    if (!manager.shutdown_all().ok() || !equals(events, {
            "initialize:root", "initialize:middle", "initialize:side",
            "initialize:leaf", "start:root", "start:middle", "start:side",
            "start:leaf", "shutdown:leaf", "shutdown:side",
            "shutdown:middle", "shutdown:root"})) {
        return false;
    }
    if (!manager.shutdown_all().ok() || events.size() != 12) {
        return false;
    }
    for (const pc::ServiceSnapshot& snapshot : manager.snapshots()) {
        if (snapshot.status.state != pc::ServiceState::stopped ||
            snapshot.status.usable) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool test_initialization_rollback_and_exceptions() {
    pb::BackendRegistry registry{};
    pc::ServiceContext context{registry};
    std::vector<std::string> events{};
    pc::ServiceManager manager{context};

    auto first = std::make_shared<RecordingService>(
        "first", std::vector<std::string>{}, events);
    Behavior throwing{};
    throwing.throw_initialize = true;
    auto second = std::make_shared<RecordingService>(
        "second", std::vector<std::string>{"first"}, events, throwing);
    if (!manager.register_service(first).ok() ||
        !manager.register_service(second).ok()) {
        return false;
    }
    const pc::ServiceManagerResult result = manager.initialize_all();
    return result.code == pc::ServiceManagerCode::initialization_failed &&
           result.failures.size() == 1 &&
           result.failures.front().code == pc::ServiceFailureCode::exception &&
           equals(events, {"initialize:first", "initialize:second",
                           "shutdown:second", "shutdown:first"}) &&
           first->status().state == pc::ServiceState::stopped &&
           second->status().state == pc::ServiceState::stopped;
}

[[nodiscard]] bool test_startup_rollback_and_best_effort_shutdown() {
    pb::BackendRegistry registry{};
    pc::ServiceContext context{registry};
    std::vector<std::string> events{};
    pc::ServiceManager manager{context};

    auto first = std::make_shared<RecordingService>(
        "first", std::vector<std::string>{}, events);
    Behavior startup_failure{};
    startup_failure.fail_start = true;
    auto second = std::make_shared<RecordingService>(
        "second", std::vector<std::string>{"first"}, events, startup_failure);
    Behavior shutdown_failure{};
    shutdown_failure.fail_shutdown = true;
    auto third = std::make_shared<RecordingService>(
        "third", std::vector<std::string>{"second"}, events,
        shutdown_failure);
    if (!manager.register_service(first).ok() ||
        !manager.register_service(second).ok() ||
        !manager.register_service(third).ok() ||
        !manager.initialize_all().ok()) {
        return false;
    }
    const pc::ServiceManagerResult result = manager.start_all();
    return result.code == pc::ServiceManagerCode::startup_failed &&
           result.failures.size() == 2 &&
           equals(events, {"initialize:first", "initialize:second",
                           "initialize:third", "start:first", "start:second",
                           "shutdown:third", "shutdown:second",
                           "shutdown:first"}) &&
           first->status().state == pc::ServiceState::stopped &&
           second->status().state == pc::ServiceState::stopped &&
           third->status().state == pc::ServiceState::failed;
}

}  // namespace

int main() {
    return test_registration_and_dependencies() &&
                   test_dependency_errors() && test_order_and_context() &&
                   test_initialization_rollback_and_exceptions() &&
                   test_startup_rollback_and_best_effort_shutdown()
               ? EXIT_SUCCESS
               : EXIT_FAILURE;
}
