#pragma once

#include "ControlTypes.hpp"

#include <optional>

namespace vpsm::server::application {
    template<typename T>
    class IRequestDecoder {
    public:
        virtual ~IRequestDecoder() = default;
        virtual std::optional<T> decode(const ControlRequest& request) = 0;
    };
}
