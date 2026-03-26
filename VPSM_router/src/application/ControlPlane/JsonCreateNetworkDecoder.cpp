#include "JsonCreateNetworkDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::CreateNetworkDto> JsonCreateNetworkDecoder::decode(const ControlRequest& request) {
        if (!json_support::hasJsonContentType(request)) {
            return std::nullopt;
        }

        const auto object = json_support::parseJsonObject(request);
        if (!object) {
            return std::nullopt;
        }

        const auto ownerPeerId = json_support::readU64(*object, "ownerPeerId");
        const auto name = json_support::readString(*object, "name");
        const auto passwordHash = json_support::readString(*object, "passwordHash");
        if (!ownerPeerId || !name || !passwordHash) {
            return std::nullopt;
        }

        return dto::CreateNetworkDto{
            .ownerPeerId = *ownerPeerId,
            .name = *name,
            .passwordHash = *passwordHash,
        };
    }
}
