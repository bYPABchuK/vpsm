#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class CreatePeerEndpoint final : public IControlEndpoint {
    public:
        CreatePeerEndpoint(
            port::IUserService& userService,
            IRequestDecoder<dto::CreatePeerDto>& requestDecoder,
            IResponseEncoder<dto::CreatePeerResultDto>& responseEncoder
        )
            : userService_(userService),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IRequestDecoder<dto::CreatePeerDto>& requestDecoder_;
        IResponseEncoder<dto::CreatePeerResultDto>& responseEncoder_;
    };
}
