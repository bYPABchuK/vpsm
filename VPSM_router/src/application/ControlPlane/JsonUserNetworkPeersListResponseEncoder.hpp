#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonUserNetworkPeersListResponseEncoder final : public IResponseEncoder<dto::UserNetworkPeersListResultDto> {
    public:
        ControlResponse encode(const dto::UserNetworkPeersListResultDto& response) override;
    };
}