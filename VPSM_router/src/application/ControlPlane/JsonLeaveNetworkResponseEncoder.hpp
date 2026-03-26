#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonLeaveNetworkResponseEncoder final : public IResponseEncoder<dto::LeaveNetworkResultDto> {
    public:
        ControlResponse encode(const dto::LeaveNetworkResultDto& response) override;
    };
}
