#pragma once

#include "ControlTypes.hpp"

namespace vpsm::server::application {
    template<typename T>
    class IResponseEncoder {
    public:
        virtual ~IResponseEncoder() = default;
        virtual ControlResponse encode(const T& response) = 0;
    };
}
