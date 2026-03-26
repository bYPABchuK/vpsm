#pragma once

#include "IControlEndpoint.hpp"

namespace vpsm::server::application {
    class HealthEndpoint final : public IControlEndpoint {
    public:
        ControlResponse handle(const ControlRequest& request) override;
    };
}
