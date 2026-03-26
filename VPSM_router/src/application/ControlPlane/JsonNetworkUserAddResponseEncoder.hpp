#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonNetworkUserAddResponseEncoder final : public IResponseEncoder<dto::NetworkUserAddResultDto> {
    public:
        ControlResponse encode(const dto::NetworkUserAddResultDto& response) override;
    };
}
