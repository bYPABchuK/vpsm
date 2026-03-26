#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonCreateNetworkResponseEncoder final : public IResponseEncoder<dto::CreateNetworkResultDto> {
    public:
        ControlResponse encode(const dto::CreateNetworkResultDto& response) override;
    };
}
