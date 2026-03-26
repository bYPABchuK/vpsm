#include "JsonCreatePeerDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::CreatePeerDto> JsonCreatePeerDecoder::decode(const ControlRequest& request) {
        if (!json_support::hasJsonContentType(request)) {
            return std::nullopt;
        }

        const auto object = json_support::parseJsonObject(request);
        if (!object) {
            return std::nullopt;
        }

        const auto nickname = json_support::readString(*object, "nickname");
        const auto passwordHash = json_support::readString(*object, "passwordHash");
        if (!nickname || !passwordHash) {
            return std::nullopt;
        }

        return dto::CreatePeerDto{
            .nickname = *nickname,
            .passwordHash = *passwordHash,
        };
    }
}
