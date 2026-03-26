#include "JsonCreatePeerDecoder.hpp"

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

    std::optional<dto::CreatePeerDto> JsonCreatePeerDecoder::decode(const ControlRequest& request) {
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
        const auto passwordHashIt = object.find("passwordHash");
        if (nicknameIt == object.end() || passwordHashIt == object.end()) {
            return std::nullopt;
        }
        if (!nicknameIt->value().is_string() || !passwordHashIt->value().is_string()) {
            return std::nullopt;
        }

        return dto::CreatePeerDto{
            .nickname = std::string(nicknameIt->value().as_string().c_str()),
            .passwordHash = std::string(passwordHashIt->value().as_string().c_str()),
        };
    }
}
