#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonNetworkPeersListResponseEncoder final : public IResponseEncoder<dto::NetworkPeersListResultDto> {
    public:
        ControlResponse encode(const dto::NetworkPeersListResultDto& response) override;
    };
}
