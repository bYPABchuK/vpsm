#include "JsonDtoDecoder.hpp"

namespace vpsm::server::application::dto {
    namespace {
        std::optional<std::string> readString(const boost::json::object& object, const char* key) {
            const auto it = object.find(key);
            if (it == object.end() || !it->value().is_string()) {
                return std::nullopt;
            }
            return std::string(it->value().as_string().c_str());
        }

        std::optional<std::uint64_t> readU64(const boost::json::object& object, const char* key) {
            const auto it = object.find(key);
            if (it == object.end() || !it->value().is_int64()) {
                return std::nullopt;
            }
            const auto value = it->value().as_int64();
            if (value < 0) {
                return std::nullopt;
            }
            return static_cast<std::uint64_t>(value);
        }

        std::optional<std::uint64_t> readPathU64(const ControlRequest& request, const char* key) {
            const auto it = request.pathParams.find(key);
            if (it == request.pathParams.end()) {
                return std::nullopt;
            }

            try {
                return static_cast<std::uint64_t>(std::stoull(it->second));
            } catch (...) {
                return std::nullopt;
            }
        }
    }

    std::optional<CreatePeerDto> JsonDtoDecoder::decodeCreatePeer(const boost::json::object& object) const {
        const auto nickname = readString(object, "nickname");
        const auto passwordHash = readString(object, "passwordHash");
        if (!nickname || !passwordHash) {
            return std::nullopt;
        }
        return CreatePeerDto{.nickname = *nickname, .passwordHash = *passwordHash};
    }

    std::optional<CreateNetworkDto> JsonDtoDecoder::decodeCreateNetwork(const boost::json::object& object) const {
        const auto ownerPeerId = readU64(object, "ownerPeerId");
        const auto name = readString(object, "name");
        const auto passwordHash = readString(object, "passwordHash");
        if (!ownerPeerId || !name || !passwordHash) {
            return std::nullopt;
        }
        return CreateNetworkDto{.ownerPeerId = *ownerPeerId, .name = *name, .passwordHash = *passwordHash};
    }

    std::optional<JoinNetworkDto> JsonDtoDecoder::decodeJoinNetwork(const boost::json::object& object, const ControlRequest& request) const {
        const auto peerId = readPathU64(request, "peerId").value_or(0);
        const auto networkId = readPathU64(request, "networkId").value_or(0);
        const auto bodyPeerId = readU64(object, "peerId");
        const auto bodyNetworkId = readU64(object, "networkId");
        const auto passwordHash = readString(object, "passwordHash");

        const auto resolvedPeerId = (peerId != 0) ? std::optional<std::uint64_t>{peerId} : bodyPeerId;
        const auto resolvedNetworkId = (networkId != 0) ? std::optional<std::uint64_t>{networkId} : bodyNetworkId;

        if (!resolvedPeerId || !resolvedNetworkId || !passwordHash) {
            return std::nullopt;
        }

        return JoinNetworkDto{.peerId = *resolvedPeerId, .networkId = *resolvedNetworkId, .passwordHash = *passwordHash};
    }

    std::optional<LeaveNetworkDto> JsonDtoDecoder::decodeLeaveNetwork(const boost::json::object& object, const ControlRequest& request) const {
        const auto peerId = readPathU64(request, "peerId").value_or(0);
        const auto networkId = readPathU64(request, "networkId").value_or(0);
        const auto bodyPeerId = readU64(object, "peerId");
        const auto bodyNetworkId = readU64(object, "networkId");

        const auto resolvedPeerId = (peerId != 0) ? std::optional<std::uint64_t>{peerId} : bodyPeerId;
        const auto resolvedNetworkId = (networkId != 0) ? std::optional<std::uint64_t>{networkId} : bodyNetworkId;

        if (!resolvedPeerId || !resolvedNetworkId) {
            return std::nullopt;
        }

        return LeaveNetworkDto{.peerId = *resolvedPeerId, .networkId = *resolvedNetworkId};
    }
}
