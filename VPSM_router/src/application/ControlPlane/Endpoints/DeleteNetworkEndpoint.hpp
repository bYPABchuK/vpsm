#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class DeleteNetworkEndpoint final : public IControlEndpoint {
    public:
        DeleteNetworkEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::LeaveNetworkResultDto>& responseEncoder
        ) : userService_(userService), responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::LeaveNetworkResultDto>& responseEncoder_;
    };
}