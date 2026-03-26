#pragma once

#include "../IControlEndpoint.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class CreateNetworkAuthEndpoint final : public IControlEndpoint {
    public:
        explicit CreateNetworkAuthEndpoint(
            port::IUserService& userService,
            IResponseEncoder<dto::CreateNetworkAuthResultDto>& responseEncoder
        )
            : userService_(userService),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IResponseEncoder<dto::CreateNetworkAuthResultDto>& responseEncoder_;
    };
}
