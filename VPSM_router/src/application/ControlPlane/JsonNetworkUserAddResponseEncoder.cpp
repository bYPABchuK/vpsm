#include "JsonNetworkUserAddResponseEncoder.hpp"

#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonNetworkUserAddResponseEncoder::encode(const dto::NetworkUserAddResultDto& response) {
        boost::json::object out;
        out["ok"] = response.ok;
        out["alreadyExists"] = response.alreadyExists;

        if (response.vip.has_value()) {
            out["vip"] = static_cast<std::int64_t>(*response.vip);
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
