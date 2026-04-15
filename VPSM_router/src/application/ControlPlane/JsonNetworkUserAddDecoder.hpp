#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonNetworkUserAddDecoder final : public IRequestDecoder<dto::NetworkUserAddDto> {
    public:
        std::optional<dto::NetworkUserAddDto> decode(const ControlRequest& request) override;
    };
}