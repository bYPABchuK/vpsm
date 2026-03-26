#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class NetworkPeersListEndpoint final : public IControlEndpoint {
    public:
        explicit NetworkPeersListEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::NetworkPeersListResultDto>& responseEncoder
        )
            : userService_(userService),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::NetworkPeersListResultDto>& responseEncoder_;
    };
}
