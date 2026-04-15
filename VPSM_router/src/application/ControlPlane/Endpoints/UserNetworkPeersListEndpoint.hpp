#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class UserNetworkPeersListEndpoint final : public IControlEndpoint {
    public:
        explicit UserNetworkPeersListEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::UserNetworkPeersListResultDto>& responseEncoder
        )
            : userService_(userService),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::UserNetworkPeersListResultDto>& responseEncoder_;
    };
}