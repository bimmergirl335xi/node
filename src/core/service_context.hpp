#ifndef SRC_CORE_SERVICE_CONTEXT_HPP
#define SRC_CORE_SERVICE_CONTEXT_HPP

namespace prometheus::backends {
class BackendRegistry;
}

namespace prometheus::core {

// ServiceContext is a non-owning view of facilities owned by the surrounding
// runtime. The BackendRegistry must outlive this context and every service
// lifecycle callback that receives it.
class ServiceContext {
public:
    explicit ServiceContext(backends::BackendRegistry& backend_registry) noexcept;

    [[nodiscard]] backends::BackendRegistry& backend_registry() const noexcept;

private:
    backends::BackendRegistry* backend_registry_ = nullptr;
};

}  // namespace prometheus::core

#endif  // SRC_CORE_SERVICE_CONTEXT_HPP
