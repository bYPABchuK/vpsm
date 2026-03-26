#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

#include <string>

namespace vpsm::server::application::endpoints {
    class JoinNetworkEndpoint final : public IControlEndpoint {
    public:
        explicit JoinNetworkEndpoint(
            port::IUserService& userService,
            IRequestDecoder<dto::JoinNetworkDto>& requestDecoder,
            IResponseEncoder<dto::JoinNetworkResultDto>& responseEncoder,
            std::string expectedMethod = "PUT"
        )
            : userService_(userService),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder),
              expectedMethod_(std::move(expectedMethod)) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IRequestDecoder<dto::JoinNetworkDto>& requestDecoder_;
        IResponseEncoder<dto::JoinNetworkResultDto>& responseEncoder_;
        std::string expectedMethod_;
    };
}
