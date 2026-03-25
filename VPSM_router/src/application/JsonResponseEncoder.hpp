#pragma once

#include "IResponseEncoder.hpp"
#include "JsonTypes.hpp"

namespace vpsm::server::application {
    class JsonResponseEncoder final : public IResponseEncoder<JsonBodyResponse> {
    public:
        ControlResponse encode(const JsonBodyResponse& response) override;
    };
}
