#include "JsonNetworkPeersListResponseEncoder.hpp"

#include <boost/json/array.hpp>
#include <boost/json/object.hpp>
#include <boost/json/serialize.hpp>

namespace vpsm::server::application {
    ControlResponse JsonNetworkPeersListResponseEncoder::encode(const dto::NetworkPeersListResultDto& response) {
        boost::json::object out;
        out["ok"] = response.ok;

        boost::json::array items;
        for (const auto& peer : response.peers) {
            boost::json::object item;
            item["peerId"] = static_cast<std::int64_t>(peer.peerId);
            item["vip"] = static_cast<std::int64_t>(peer.vip);
            items.push_back(std::move(item));
        }
        out["peers"] = std::move(items);

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
