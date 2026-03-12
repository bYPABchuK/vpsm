#include "TcpManagerBoost.hpp"

namespace vpsm::server::adapter::boostImpl {
    int TcpManagerBoost::send(domain::PacketOut pkt) {
        (void)pkt;
        return -1;
    }
}