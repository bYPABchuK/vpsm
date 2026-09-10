#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class JoinNetworkByNameEndpoint final : public IControlEndpoint {
    public:
        JoinNetworkByNameEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::NetworkUserAddResultDto>& responseEncoder
        ) : userService_(userService), responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::NetworkUserAddResultDto>& responseEncoder_;
    };
}