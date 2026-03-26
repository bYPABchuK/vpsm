#include "JsonLoginDecoder.hpp"
#include "JsonSupport.hpp"

namespace vpsm::server::application {
    std::optional<dto::LoginDto> JsonLoginDecoder::decode(const ControlRequest& request) {
        if (!json_support::hasJsonContentType(request)) {
            return std::nullopt;
        }

        const auto object = json_support::parseJsonObject(request);
        if (!object) {
            return std::nullopt;
        }

        const auto nickname = json_support::readString(*object, "nickname");
        const auto nick = json_support::readString(*object, "nick");
        if (!nickname && !nick) {
            return std::nullopt;
        }

        const auto passwordIt = object->find("passwordHash");
        const auto passwordRawIt = object->find("password");
        if (passwordIt != object->end() && !passwordIt->value().is_string()) {
            return std::nullopt;
        }
        if (passwordIt == object->end() && passwordRawIt != object->end() && !passwordRawIt->value().is_string()) {
            return std::nullopt;
        }

        std::string passwordHash;
        if (const auto password = json_support::readString(*object, "passwordHash")) {
            passwordHash = *password;
        } else if (const auto passwordRaw = json_support::readString(*object, "password")) {
            passwordHash = *passwordRaw;
        }

        return dto::LoginDto{
            .nickname = nickname ? *nickname : *nick,
            .passwordHash = std::move(passwordHash),
        };
    }
}
