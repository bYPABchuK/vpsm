#include "JsonJoinNetworkDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::JoinNetworkDto> JsonJoinNetworkDecoder::decode(const ControlRequest& request) {
        if (!json_support::hasJsonContentType(request)) {
            return std::nullopt;
        }

        const auto object = json_support::parseJsonObject(request);
        if (!object) {
            return std::nullopt;
        }

        const auto passwordHash = json_support::readString(*object, "passwordHash");
        if (!passwordHash) {
            return std::nullopt;
        }

        const auto pathPeerId = json_support::readPathU64(request, "peerId");
        const auto bodyPeerId = json_support::readU64(*object, "peerId");
        const auto resolvedPeerId = pathPeerId ? pathPeerId : bodyPeerId;

        const auto pathNetworkId = json_support::readPathU64(request, "networkId");
        const auto bodyNetworkId = json_support::readU64(*object, "networkId");
        const auto resolvedNetworkId = pathNetworkId ? pathNetworkId : bodyNetworkId;

        if (!resolvedPeerId.has_value() || !resolvedNetworkId.has_value()) {
            return std::nullopt;
        }

        return dto::JoinNetworkDto{
            .peerId = *resolvedPeerId,
            .networkId = *resolvedNetworkId,
            .passwordHash = *passwordHash,
        };
    }
}
