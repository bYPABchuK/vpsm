#pragma once

#include "../IControlEndpoint.hpp"
#include "../SessionStore.hpp"

namespace vpsm::server::application::endpoints {
    class LogoutEndpoint final : public IControlEndpoint {
    public:
        explicit LogoutEndpoint(SessionStore& sessionStore)
            : sessionStore_(sessionStore) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        SessionStore& sessionStore_;
    };
}