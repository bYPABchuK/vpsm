#pragma once

#include "IResponseEncoder.hpp"
#include "../DTO/UserDtos.hpp"

namespace vpsm::server::application {
    class JsonLoginResponseEncoder final : public IResponseEncoder<dto::LoginResultDto> {
    public:
        ControlResponse encode(const dto::LoginResultDto& response) override;
    };
}
