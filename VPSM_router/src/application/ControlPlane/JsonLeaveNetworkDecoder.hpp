#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonLeaveNetworkDecoder final : public IRequestDecoder<dto::LeaveNetworkDto> {
    public:
        std::optional<dto::LeaveNetworkDto> decode(const ControlRequest& request) override;
    };
}
