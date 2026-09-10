#include "JsonUserNetworkListResponseEncoder.hpp"
#include "JsonSupport.hpp"
#include "../../domain/type/overlayNetwork.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonUserNetworkListResponseEncoder::encode(const dto::UserNetworkListResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        boost::json::array items;
        for (const auto& network : response.networks) {
            boost::json::object item;
            item["id"] = std::to_string(network.id);
            item["name"] = network.name;
            item["ownerPeerId"] = std::to_string(network.ownerPeerId);
            item["address"] = domain::ipv4ToString(network.localVip);
            item["vip"] = domain::ipv4ToString(network.localVip);
            item["networkAddress"] = domain::ipv4ToString(network.networkAddress);
            item["prefixLength"] = network.prefixLength;
            item["mtu"] = network.mtu;
            items.push_back(std::move(item));
        }
        out["networks"] = std::move(items);

        return json_support::toJsonResponse(response.status, out);
    }
}
