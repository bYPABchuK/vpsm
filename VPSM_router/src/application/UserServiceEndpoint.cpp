#include "UserServiceEndpoint.hpp"

#include <boost/json/object.hpp>

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

        ControlResponse badRequest(JsonResponseEncoder& encoder, const char* error) {
            boost::json::object out;
            out["ok"] = false;
            out["error"] = error;
            return encoder.encode(JsonBodyResponse{.status = 400, .body = out});
        }

        std::optional<std::uint64_t> readU64(const boost::json::object& o, const char* key) {
            const auto it = o.find(key);
            if (it == o.end() || !it->value().is_int64()) {
                return std::nullopt;
            }
            const auto v = it->value().as_int64();
            if (v < 0) {
                return std::nullopt;
            }
            return static_cast<std::uint64_t>(v);
        }

        std::optional<std::string> readString(const boost::json::object& o, const char* key) {
            const auto it = o.find(key);
            if (it == o.end() || !it->value().is_string()) {
                return std::nullopt;
            }
            return std::string(it->value().as_string().c_str());
        }
    }

    ControlResponse UserServiceEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return ControlResponse{.status = 405, .contentType = "text/plain", .body = {'m','e','t','h','o','d','_','n','o','t','_','a','l','l','o','w','e','d'}};
        }

        if (!isJsonContentType(request)) {
            return ControlResponse{.status = 415, .contentType = "text/plain", .body = {'u','n','s','u','p','p','o','r','t','e','d','_','m','e','d','i','a','_','t','y','p','e'}};
        }

        const auto payload = decoder_.decode(request);
        if (!payload.has_value() || !payload->is_object()) {
            return badRequest(encoder_, "invalid_json");
        }

        const auto& obj = payload->as_object();
        boost::json::object out;

        switch (operation_) {
            case Operation::CREATE_PEER: {
                auto nickname = readString(obj, "nickname");
                auto passwordHash = readString(obj, "passwordHash");
                if (!nickname || !passwordHash) {
                    return badRequest(encoder_, "invalid_payload");
                }

                const auto peerId = userService_.createPeer(*nickname, *passwordHash);
                if (!peerId) {
                    out["ok"] = false;
                    return encoder_.encode(JsonBodyResponse{.status = 400, .body = out});
                }

                out["ok"] = true;
                out["peerId"] = static_cast<std::int64_t>(*peerId);
                return encoder_.encode(JsonBodyResponse{.status = 200, .body = out});
            }
            case Operation::CREATE_NETWORK: {
                auto ownerPeerId = readU64(obj, "ownerPeerId");
                auto name = readString(obj, "name");
                auto passwordHash = readString(obj, "passwordHash");
                if (!ownerPeerId || !name || !passwordHash) {
                    return badRequest(encoder_, "invalid_payload");
                }

                const auto networkId = userService_.createNetwork(*ownerPeerId, *name, *passwordHash);
                if (!networkId) {
                    out["ok"] = false;
                    return encoder_.encode(JsonBodyResponse{.status = 400, .body = out});
                }

                out["ok"] = true;
                out["networkId"] = static_cast<std::int64_t>(*networkId);
                return encoder_.encode(JsonBodyResponse{.status = 200, .body = out});
            }
            case Operation::JOIN_NETWORK: {
                auto peerId = readU64(obj, "peerId");
                auto networkId = readU64(obj, "networkId");
                auto passwordHash = readString(obj, "passwordHash");
                if (!peerId || !networkId || !passwordHash) {
                    return badRequest(encoder_, "invalid_payload");
                }

                const auto vip = userService_.joinNetwork(*peerId, *networkId, *passwordHash);
                if (!vip) {
                    out["ok"] = false;
                    return encoder_.encode(JsonBodyResponse{.status = 400, .body = out});
                }

                out["ok"] = true;
                out["vip"] = static_cast<std::int64_t>(*vip);
                return encoder_.encode(JsonBodyResponse{.status = 200, .body = out});
            }
            case Operation::LEAVE_NETWORK: {
                auto peerId = readU64(obj, "peerId");
                auto networkId = readU64(obj, "networkId");
                if (!peerId || !networkId) {
                    return badRequest(encoder_, "invalid_payload");
                }

                const auto ok = userService_.leaveNetwork(*peerId, *networkId);
                out["ok"] = ok;
                return encoder_.encode(JsonBodyResponse{.status = 200, .body = out});
            }
        }

        return badRequest(encoder_, "unsupported_operation");
    }
}
