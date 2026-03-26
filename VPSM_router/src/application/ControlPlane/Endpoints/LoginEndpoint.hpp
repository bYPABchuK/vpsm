#pragma once

#include "../IControlEndpoint.hpp"
#include "../IRequestDecoder.hpp"
#include "../IResponseEncoder.hpp"
#include "../SessionStore.hpp"
#include "../../../port/IUserService.hpp"
#include "../../DTO/UserDtos.hpp"

namespace vpsm::server::application::endpoints {
    class LoginEndpoint final : public IControlEndpoint {
    public:
        LoginEndpoint(
            port::IUserService& userService,
            SessionStore& sessionStore,
            IRequestDecoder<dto::LoginDto>& requestDecoder,
            IResponseEncoder<dto::LoginResultDto>& responseEncoder
        )
            : userService_(userService),
              sessionStore_(sessionStore),
              requestDecoder_(requestDecoder),
              responseEncoder_(responseEncoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        SessionStore& sessionStore_;
        IRequestDecoder<dto::LoginDto>& requestDecoder_;
        IResponseEncoder<dto::LoginResultDto>& responseEncoder_;
    };
}
