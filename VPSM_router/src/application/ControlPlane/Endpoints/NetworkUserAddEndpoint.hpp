#pragma once

#include "../IControlEndpoint.hpp"
#include "../../../port/IUserService.hpp"

namespace vpsm::server::application::endpoints {
    class NetworkUserAddEndpoint final : public IControlEndpoint {
    public:
        explicit NetworkUserAddEndpoint(port::IUserService& userService)
            : userService_(userService) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
    };
}
