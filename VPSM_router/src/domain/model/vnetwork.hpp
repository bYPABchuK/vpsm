#pragma once

#include "peer.hpp"
#include <string>
#include <vector>
namespace vpsm::server::domain {
    using VNetworkId = unsigned long;

    struct VNetwork {
        VNetworkId id = 0;
        std::string name;
        std::string password_hash;
        std::uint32_t networkAddress = 0;
        std::uint8_t prefixLength = 24;
        std::uint16_t mtu = 1400;
        std::vector<std::string> cipher_suites{
            "none",
            "CHACHA20-POLY1305",
            "AES-256-GCM",
        };
        Peer owner;
    };

    struct NetworkMembership {
        VNetwork network;
        std::uint32_t localVip = 0;
    };
}