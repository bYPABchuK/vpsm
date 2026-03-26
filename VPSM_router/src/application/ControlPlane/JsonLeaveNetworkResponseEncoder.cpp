#include "JsonLeaveNetworkResponseEncoder.hpp"

#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonLeaveNetworkResponseEncoder::encode(const dto::LeaveNetworkResultDto& response) {
        boost::json::object out;
        out["ok"] = response.ok;

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
