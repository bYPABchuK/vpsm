#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonCreateNetworkAuthResponseEncoder final : public IResponseEncoder<dto::CreateNetworkAuthResultDto> {
    public:
        ControlResponse encode(const dto::CreateNetworkAuthResultDto& response) override;
    };
}
