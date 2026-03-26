#include "JsonResponseEncoder.hpp"

#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonResponseEncoder::encode(const JsonBodyResponse& response) {
        const auto bodyText = boost::json::serialize(response.body);

        return ControlResponse{
            .status = response.status,
            .contentType = "application/json",
            .body = std::vector<std::uint8_t>(bodyText.begin(), bodyText.end()),
        };
    }
}
