#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonJoinNetworkResponseEncoder final : public IResponseEncoder<dto::JoinNetworkResultDto> {
    public:
        ControlResponse encode(const dto::JoinNetworkResultDto& response) override;
    };
}
