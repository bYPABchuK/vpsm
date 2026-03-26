#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class UserNetworkListEndpoint final : public IControlEndpoint {
    public:
        explicit UserNetworkListEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::UserNetworkListResultDto>& responseEncoder
        )
            : userService_(userService),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::UserNetworkListResultDto>& responseEncoder_;
    };
}
