#include "JsonLoginDecoder.hpp"

#include <boost/json/parse.hpp>
#include <boost/system/error_code.hpp>

namespace vpsm::server::application {
    namespace {
        bool isJsonContentType(const ControlRequest& request) {
            const auto it = request.headers.find("Content-Type");
            if (it != request.headers.end() && it->second.find("application/json") != std::string::npos) {
                return true;
            }

            const auto lowerIt = request.headers.find("content-type");
            return lowerIt != request.headers.end() && lowerIt->second.find("application/json") != std::string::npos;
        }
    }

    std::optional<dto::LoginDto> JsonLoginDecoder::decode(const ControlRequest& request) {
        if (!isJsonContentType(request)) {
            return std::nullopt;
        }

        const std::string jsonText(request.body.begin(), request.body.end());

        boost::system::error_code ec;
        const auto value = boost::json::parse(jsonText, ec);
        if (ec || !value.is_object()) {
            return std::nullopt;
        }

        const auto& object = value.as_object();
        const auto nicknameIt = object.find("nickname");
        const auto nickIt = object.find("nick");
        if ((nicknameIt == object.end() || !nicknameIt->value().is_string())
            && (nickIt == object.end() || !nickIt->value().is_string())) {
            return std::nullopt;
        }

        const auto passwordIt = object.find("passwordHash");
        const auto passwordRawIt = object.find("password");
        std::string passwordHash;
        if (passwordIt != object.end()) {
            if (!passwordIt->value().is_string()) {
                return std::nullopt;
            }
            passwordHash = std::string(passwordIt->value().as_string().c_str());
        } else if (passwordRawIt != object.end()) {
            if (!passwordRawIt->value().is_string()) {
                return std::nullopt;
            }
            passwordHash = std::string(passwordRawIt->value().as_string().c_str());
        }

        const std::string nickname = (nicknameIt != object.end() && nicknameIt->value().is_string())
            ? std::string(nicknameIt->value().as_string().c_str())
            : std::string(nickIt->value().as_string().c_str());

        return dto::LoginDto{
            .nickname = nickname,
            .passwordHash = std::move(passwordHash),
        };
    }
}
