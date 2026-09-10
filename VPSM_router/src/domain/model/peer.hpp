#pragma once

#include <cstdint>
#include <string>
namespace vpsm::server::domain {
    using PeerId = unsigned long;

    struct Peer {
        PeerId peerId;
        uint32_t vip;
        std::string nickname;
    };
}