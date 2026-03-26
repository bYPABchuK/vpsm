#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

#include <string>

namespace vpsm::server::application::endpoints {
    class LeaveNetworkEndpoint final : public IControlEndpoint {
    public:
        explicit LeaveNetworkEndpoint(
            port::IUserService& userService,
            IRequestDecoder<dto::LeaveNetworkDto>& requestDecoder,
            IResponseEncoder<dto::LeaveNetworkResultDto>& responseEncoder,
            std::string expectedMethod = "DELETE"
        )
            : userService_(userService),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder),
              expectedMethod_(std::move(expectedMethod)) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        IRequestDecoder<dto::LeaveNetworkDto>& requestDecoder_;
        IResponseEncoder<dto::LeaveNetworkResultDto>& responseEncoder_;
        std::string expectedMethod_;
    };
}
