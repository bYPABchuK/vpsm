#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonCreatePeerResponseEncoder final : public IResponseEncoder<dto::CreatePeerResultDto> {
    public:
        ControlResponse encode(const dto::CreatePeerResultDto& response) override;
    };
}
