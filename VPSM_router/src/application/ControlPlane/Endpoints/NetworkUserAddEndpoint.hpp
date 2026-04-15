#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class NetworkUserAddEndpoint final : public IControlEndpoint {
    public:
        explicit NetworkUserAddEndpoint(
            port::IUserService& userService,
            IRequestDecoder<dto::NetworkUserAddDto>& requestDecoder,
            IResponseEncoder<dto::NetworkUserAddResultDto>& responseEncoder
        )
            : userService_(userService),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IRequestDecoder<dto::NetworkUserAddDto>& requestDecoder_;
        IResponseEncoder<dto::NetworkUserAddResultDto>& responseEncoder_;
    };
}
