#include "LoginEndpoint.hpp"
#include "UserServiceErrorMapper.hpp"

namespace vpsm::server::application::endpoints {
    ControlResponse LoginEndpoint::handle(const ControlRequest& request) {
        if (request.method != "POST") {
            return responseEncoder_.encode(dto::LoginResultDto{.ok = false, .status = 405, .error = "method_not_allowed"});
        }

        const auto dto = requestDecoder_.decode(request);
        if (!dto.has_value()) {
            return responseEncoder_.encode(dto::LoginResultDto{.ok = false, .status = 400, .error = "invalid_payload"});
        }

        if (dto->nickname.empty()) {
            return responseEncoder_.encode(dto::LoginResultDto{.ok = false, .status = 400, .error = "nickname_required"});
        }

        std::optional<std::uint64_t> peerId = userService_.findPeerIdByNickname(dto->nickname);
        if (!peerId.has_value()) {
            const auto createResult = userService_.createPeer(dto->nickname, dto->passwordHash);
            if (std::holds_alternative<port::UserServiceError>(createResult)) {
                const auto error = std::get<port::UserServiceError>(createResult);
                return responseEncoder_.encode(dto::LoginResultDto{
                    .ok = false,
                    .status = user_service_error_mapper::toStatus(error),
                    .error = user_service_error_mapper::toErrorString(error),
                });
            }

            peerId = std::get<port::CreatePeerSuccess>(createResult).peerId;
        } else {
            if (!userService_.verifyPeerPassword(*peerId, dto->passwordHash)) {
                return responseEncoder_.encode(dto::LoginResultDto{.ok = false, .status = 403, .error = "invalid_password"});
            }
        }

        const auto session = sessionStore_.createSession(*peerId);
        return responseEncoder_.encode(dto::LoginResultDto{
            .ok = true,
            .status = 200,
            .peerId = *peerId,
            .sessionId = session.sessionId,
            .sessionKey = session.sessionKey,
        });
    }
}
