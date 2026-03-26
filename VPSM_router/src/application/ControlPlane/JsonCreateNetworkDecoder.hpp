#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonCreateNetworkDecoder final : public IRequestDecoder<dto::CreateNetworkDto> {
    public:
        std::optional<dto::CreateNetworkDto> decode(const ControlRequest& request) override;
    };
}
