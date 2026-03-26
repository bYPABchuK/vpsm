#pragma once

#include "IControlEndpoint.hpp"
#include "JsonRequestDecoder.hpp"
#include "JsonResponseEncoder.hpp"

namespace vpsm::server::application {
    class DispatchEchoEndpoint final : public IControlEndpoint {
    public:
        ControlResponse handle(const ControlRequest& request) override;

    private:
        JsonRequestDecoder decoder_;
        JsonResponseEncoder encoder_;
    };
}
