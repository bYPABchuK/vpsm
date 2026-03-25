#pragma once

#include "ControlTypes.hpp"

namespace vpsm::server::application {
    class IControlEndpoint {
    public:
        virtual ~IControlEndpoint() = default;
        virtual ControlResponse handle(const ControlRequest& request) = 0;
    };
}
