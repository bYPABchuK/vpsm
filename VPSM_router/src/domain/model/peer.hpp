#pragma once

#include <cstdint>
namespace vpsm::server::domain {
    using PeerId = unsigned long;

    struct Peer {
        PeerId peerId;
        uint32_t vip;
    };
}