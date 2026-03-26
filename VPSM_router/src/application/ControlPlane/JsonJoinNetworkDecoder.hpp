#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonJoinNetworkDecoder final : public IRequestDecoder<dto::JoinNetworkDto> {
    public:
        std::optional<dto::JoinNetworkDto> decode(const ControlRequest& request) override;
    };
}
