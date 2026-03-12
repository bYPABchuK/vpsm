#pragma once

#include "../domain/model/packetOut.hpp"

namespace vpsm::server::port {
    class IPacketSender {
        public:
        virtual int send(server::domain::PacketOut pkt) = 0;
    };
}