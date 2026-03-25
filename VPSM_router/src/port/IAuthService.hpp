#pragma once

#include "../domain/model/headerV1.hpp"
#include "../domain/model/packetIn.hpp"
namespace vpsm::server::port {
    class IAuthService {
    public:
        virtual ~IAuthService() = default;
        virtual bool verify(const domain::PacketHeader& header, const domain::PacketIn& packet) = 0;
    };
}
