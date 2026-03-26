#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class CreateNetworkEndpoint final : public IControlEndpoint {
    public:
        CreateNetworkEndpoint(
            port::IUserService& userService,
            IRequestDecoder<dto::CreateNetworkDto>& requestDecoder,
            IResponseEncoder<dto::CreateNetworkResultDto>& responseEncoder
        )
            : userService_(userService),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IRequestDecoder<dto::CreateNetworkDto>& requestDecoder_;
        IResponseEncoder<dto::CreateNetworkResultDto>& responseEncoder_;
    };
}
