#pragma once

#include "../../port/IAuthServiceV2.hpp"
#include "../ControlPlane/SessionStore.hpp"

namespace vpsm::server::application {
    class AuthServiceV2 final : public port::IAuthServiceV2 {
    public:
        explicit AuthServiceV2(SessionStore& sessionStore)
            : sessionStore_(sessionStore) {}

        std::optional<domain::AuthResultV2> verifyAndDecrypt(const domain::PacketIn& packet) override;
        std::optional<domain::buffer> encryptForSession(
            std::uint64_t sessionId,
            const std::vector<std::uint8_t>& plaintextInnerAndPayload
        ) override;

    private:
        SessionStore& sessionStore_;
    };
}