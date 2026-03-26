#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonUserNetworkListResponseEncoder final : public IResponseEncoder<dto::UserNetworkListResultDto> {
    public:
        ControlResponse encode(const dto::UserNetworkListResultDto& response) override;
    };
}
