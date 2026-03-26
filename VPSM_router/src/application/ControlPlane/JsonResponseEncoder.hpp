#pragma once

#include "IResponseEncoder.hpp"
#include "JsonTypes.hpp"
#include "../../port/IControlResponseEncoder.hpp"

namespace vpsm::server::application {
    class JsonResponseEncoder final
        : public IResponseEncoder<JsonBodyResponse>
        , public port::IControlResponseEncoder {
    public:
        ControlResponse encode(const JsonBodyResponse& response) override;
    };
}
