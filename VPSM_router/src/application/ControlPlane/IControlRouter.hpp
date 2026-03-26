#pragma once

#include "ControlTypes.hpp"

namespace vpsm::server::application {
    class IControlRouter {
    public:
        virtual ~IControlRouter() = default;
        virtual ControlResponse route(const ControlRequest& request) = 0;
    };
}
