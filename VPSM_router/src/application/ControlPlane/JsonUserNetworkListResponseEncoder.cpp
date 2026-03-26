#include "JsonUserNetworkListResponseEncoder.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonUserNetworkListResponseEncoder::encode(const dto::UserNetworkListResultDto& response) {
        boost::json::object out;
        out["ok"] = response.ok;

        boost::json::array items;
        for (const auto& network : response.networks) {
            boost::json::object item;
            item["id"] = static_cast<std::int64_t>(network.id);
            item["name"] = network.name;
            item["ownerPeerId"] = static_cast<std::int64_t>(network.ownerPeerId);
            items.push_back(std::move(item));
        }
        out["networks"] = std::move(items);

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
