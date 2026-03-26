#pragma once

#include "../application/ControlPlane/ControlTypes.hpp"
#include "../application/ControlPlane/JsonTypes.hpp"

namespace vpsm::server::port {
    class IControlResponseEncoder {
    public:
        virtual ~IControlResponseEncoder() = default;
        virtual application::ControlResponse encode(const application::JsonBodyResponse& response) = 0;
    };
}
