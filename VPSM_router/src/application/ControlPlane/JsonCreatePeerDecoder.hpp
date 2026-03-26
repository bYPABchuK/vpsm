#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonCreatePeerDecoder final : public IRequestDecoder<dto::CreatePeerDto> {
    public:
        std::optional<dto::CreatePeerDto> decode(const ControlRequest& request) override;
    };
}
