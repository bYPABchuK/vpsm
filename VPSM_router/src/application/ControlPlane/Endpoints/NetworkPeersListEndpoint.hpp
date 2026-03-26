#pragma once

#include "../IControlEndpoint.hpp"
#include "../../../port/IUserService.hpp"

namespace vpsm::server::application::endpoints {
    class NetworkPeersListEndpoint final : public IControlEndpoint {
    public:
        explicit NetworkPeersListEndpoint(port::IUserService& userService)
            : userService_(userService) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
    };
}
