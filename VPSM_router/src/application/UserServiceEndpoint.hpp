#pragma once

#include "IControlEndpoint.hpp"
#include "JsonRequestDecoder.hpp"
#include "JsonResponseEncoder.hpp"
#include "../port/IUserService.hpp"

namespace vpsm::server::application {
    class UserServiceEndpoint final : public IControlEndpoint {
    public:
        enum class Operation {
            CREATE_PEER,
            CREATE_NETWORK,
            JOIN_NETWORK,
            LEAVE_NETWORK,
        };

        UserServiceEndpoint(port::IUserService& userService, Operation operation)
            : userService_(userService), operation_(operation) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IUserService& userService_;
        Operation operation_;
        JsonRequestDecoder decoder_;
        JsonResponseEncoder encoder_;
    };
}
