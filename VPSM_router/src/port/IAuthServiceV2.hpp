#pragma once

#include "../domain/model/packetIn.hpp"
#include "../domain/type/authV2.hpp"

#include <optional>

namespace vpsm::server::port {
    class IAuthServiceV2 {
    public:
        virtual ~IAuthServiceV2() = default;

        virtual std::optional<domain::AuthResultV2> verifyAndDecrypt(const domain::PacketIn& packet) = 0;
    };
}
