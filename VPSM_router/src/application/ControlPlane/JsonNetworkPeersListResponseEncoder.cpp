#include "JsonNetworkPeersListResponseEncoder.hpp"
#include "JsonSupport.hpp"
#include "../../domain/type/overlayNetwork.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>

namespace vpsm::server::application {
    ControlResponse JsonNetworkPeersListResponseEncoder::encode(const dto::NetworkPeersListResultDto& response) {
        auto out = json_support::makeBaseResult(response.ok, response.error);

        boost::json::array items;
        for (const auto& peer : response.peers) {
            boost::json::object item;
            item["peerId"] = std::to_string(peer.peerId);
            item["vip"] = domain::ipv4ToString(peer.vip);
            item["nickname"] = peer.nickname;
            items.push_back(std::move(item));
        }
        out["peers"] = std::move(items);

        return json_support::toJsonResponse(response.status, out);
    }
}
