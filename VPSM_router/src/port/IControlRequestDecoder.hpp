#pragma once

#include "../application/ControlPlane/ControlTypes.hpp"

#include <boost/json/value.hpp>

#include <optional>

namespace vpsm::server::port {
    class IControlRequestDecoder {
    public:
        virtual ~IControlRequestDecoder() = default;
        virtual std::optional<boost::json::value> decode(const application::ControlRequest& request) = 0;
    };
}
