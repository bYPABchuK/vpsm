#pragma once

#include "IControlEndpoint.hpp"
#include "../../port/IControlRequestDecoder.hpp"
#include "../../port/IControlResponseEncoder.hpp"

namespace vpsm::server::application {
    class DispatchEchoEndpoint final : public IControlEndpoint {
    public:
        DispatchEchoEndpoint(
            port::IControlRequestDecoder& decoder,
            port::IControlResponseEncoder& encoder
        )
            : decoder_(decoder),
              encoder_(encoder) {}

        ControlResponse handle(const ControlRequest& request) override;

    private:
        port::IControlRequestDecoder& decoder_;
        port::IControlResponseEncoder& encoder_;
    };
}
