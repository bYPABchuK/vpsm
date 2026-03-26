#pragma once

#include "IRequestDecoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonLoginDecoder final : public IRequestDecoder<dto::LoginDto> {
    public:
        std::optional<dto::LoginDto> decode(const ControlRequest& request) override;
    };
}
