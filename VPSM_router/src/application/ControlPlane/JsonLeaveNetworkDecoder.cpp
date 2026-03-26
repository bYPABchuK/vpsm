#include "JsonLeaveNetworkDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::LeaveNetworkDto> JsonLeaveNetworkDecoder::decode(const ControlRequest& request) {
        if (!json_support::hasJsonContentType(request)) {
            return std::nullopt;
        }

        const auto object = json_support::parseJsonObject(request);
        if (!object) {
            return std::nullopt;
        }

        const auto bodyPeerId = json_support::readU64(*object, "peerId");
        const auto bodyNetworkId = json_support::readU64(*object, "networkId");

        const auto pathPeerId = json_support::readPathU64(request, "peerId");
        const auto pathNetworkId = json_support::readPathU64(request, "networkId");
        const auto resolvedPeerId = pathPeerId ? pathPeerId : bodyPeerId;
        const auto resolvedNetworkId = pathNetworkId ? pathNetworkId : bodyNetworkId;

        if (!resolvedPeerId.has_value() || !resolvedNetworkId.has_value()) {
            return std::nullopt;
        }

        return dto::LeaveNetworkDto{.peerId = *resolvedPeerId, .networkId = *resolvedNetworkId};
    }
}
