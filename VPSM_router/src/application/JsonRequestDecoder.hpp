#pragma once

#include "IRequestDecoder.hpp"

#include <boost/json/value.hpp>

namespace vpsm::server::application {
    class JsonRequestDecoder final : public IRequestDecoder<boost::json::value> {
    public:
        std::optional<boost::json::value> decode(const ControlRequest& request) override;
    };
}
