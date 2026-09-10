#pragma once

#include "../domain/model/packetIn.hpp"
#include "../domain/type/authV2.hpp"

#include <optional>
#include <vector>

namespace vpsm::server::port {
    class IAuthServiceV2 {
    public:
        virtual ~IAuthServiceV2() = default;

        virtual std::optional<domain::AuthResultV2> verifyAndDecrypt(const domain::PacketIn& packet) = 0;
        virtual std::optional<domain::buffer> encryptForSession(
            std::uint64_t sessionId,
            const std::vector<std::uint8_t>& plaintextInnerAndPayload
        ) = 0;
    };
}
