#include "JsonCreateNetworkDecoder.hpp"

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

    std::optional<dto::CreateNetworkDto> JsonCreateNetworkDecoder::decode(const ControlRequest& request) {
        if (!isJsonContentType(request)) {
            return std::nullopt;
        }

        boost::system::error_code ec;
        const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
        if (ec || !value.is_object()) {
            return std::nullopt;
        }

        const auto& object = value.as_object();
        const auto ownerPeerIdIt = object.find("ownerPeerId");
        const auto nameIt = object.find("name");
        const auto passwordHashIt = object.find("passwordHash");
        if (ownerPeerIdIt == object.end() || nameIt == object.end() || passwordHashIt == object.end()) {
            return std::nullopt;
        }
        if (!ownerPeerIdIt->value().is_int64() || !nameIt->value().is_string() || !passwordHashIt->value().is_string()) {
            return std::nullopt;
        }

        const auto ownerPeerId = ownerPeerIdIt->value().as_int64();
        if (ownerPeerId < 0) {
            return std::nullopt;
        }

        return dto::CreateNetworkDto{
            .ownerPeerId = static_cast<std::uint64_t>(ownerPeerId),
            .name = std::string(nameIt->value().as_string().c_str()),
            .passwordHash = std::string(passwordHashIt->value().as_string().c_str()),
        };
    }
}
