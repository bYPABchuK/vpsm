#include "JsonCreateNetworkAuthResponseEncoder.hpp"

#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonCreateNetworkAuthResponseEncoder::encode(const dto::CreateNetworkAuthResultDto& response) {
        boost::json::object out;
        out["ok"] = response.ok;

        if (response.networkId.has_value()) {
            out["networkId"] = static_cast<std::int64_t>(*response.networkId);
        }
        if (response.error.has_value()) {
            out["error"] = *response.error;
        }

        const auto text = boost::json::serialize(out);
        return ControlResponse{
            .status = response.status,
            .contentType = "application/json",
            .body = std::vector<std::uint8_t>(text.begin(), text.end()),
        };
    }
}
