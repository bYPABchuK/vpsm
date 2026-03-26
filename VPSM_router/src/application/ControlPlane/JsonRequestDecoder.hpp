#pragma once

#include "IRequestDecoder.hpp"
#include "../../port/IControlRequestDecoder.hpp"

#include <boost/json/value.hpp>

namespace vpsm::server::application {
    class JsonRequestDecoder final
        : public IRequestDecoder<boost::json::value>
        , public port::IControlRequestDecoder {
    public:
        std::optional<boost::json::value> decode(const ControlRequest& request) override;
    };
}
