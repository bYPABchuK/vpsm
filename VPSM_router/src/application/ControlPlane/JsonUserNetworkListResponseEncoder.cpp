#include "JsonUserNetworkListResponseEncoder.hpp"
#include "JsonSupport.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonUserNetworkListResponseEncoder::encode(const dto::UserNetworkListResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        boost::json::array items;
        for (const auto& network : response.networks) {
            boost::json::object item;
            item["id"] = static_cast<std::int64_t>(network.id);
            item["name"] = network.name;
            item["ownerPeerId"] = static_cast<std::int64_t>(network.ownerPeerId);
            items.push_back(std::move(item));
        }
        out["networks"] = std::move(items);

        return json_support::toJsonResponse(response.status, out);
    }
}
