#include "JsonLeaveNetworkDecoder.hpp"

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

        std::optional<std::uint64_t> readPathU64(const ControlRequest& request, const char* key) {
            const auto it = request.pathParams.find(key);
            if (it == request.pathParams.end()) return std::nullopt;
            try {
                return static_cast<std::uint64_t>(std::stoull(it->second));
            } catch (...) {
                return std::nullopt;
            }
        }
    }

    std::optional<dto::LeaveNetworkDto> JsonLeaveNetworkDecoder::decode(const ControlRequest& request) {
        if (!isJsonContentType(request)) {
            return std::nullopt;
        }

        boost::system::error_code ec;
        const auto value = boost::json::parse(std::string(request.body.begin(), request.body.end()), ec);
        if (ec || !value.is_object()) {
            return std::nullopt;
        }

        const auto& object = value.as_object();
        const auto bodyPeerIdIt = object.find("peerId");
        const auto bodyNetworkIdIt = object.find("networkId");

        std::optional<std::uint64_t> bodyPeerId;
        if (bodyPeerIdIt != object.end() && bodyPeerIdIt->value().is_int64() && bodyPeerIdIt->value().as_int64() >= 0) {
            bodyPeerId = static_cast<std::uint64_t>(bodyPeerIdIt->value().as_int64());
        }

        std::optional<std::uint64_t> bodyNetworkId;
        if (bodyNetworkIdIt != object.end() && bodyNetworkIdIt->value().is_int64() && bodyNetworkIdIt->value().as_int64() >= 0) {
            bodyNetworkId = static_cast<std::uint64_t>(bodyNetworkIdIt->value().as_int64());
        }

        const auto pathPeerId = readPathU64(request, "peerId");
        const auto pathNetworkId = readPathU64(request, "networkId");
        const auto resolvedPeerId = pathPeerId ? pathPeerId : bodyPeerId;
        const auto resolvedNetworkId = pathNetworkId ? pathNetworkId : bodyNetworkId;

        if (!resolvedPeerId.has_value() || !resolvedNetworkId.has_value()) {
            return std::nullopt;
        }

        return dto::LeaveNetworkDto{.peerId = *resolvedPeerId, .networkId = *resolvedNetworkId};
    }
}
