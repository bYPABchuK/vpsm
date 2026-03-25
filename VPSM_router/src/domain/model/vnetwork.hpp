#pragma once

#include "peer.hpp"
#include <string>
#include <vector>
namespace vpsm::server::domain {
    using VNetworkId = unsigned long;

    struct VNetwork {
        VNetworkId id;
        std::string name;
        std::string password_hash;
        std::vector<std::string> cipher_suites{
            "none",
            "CHACHA20-POLY1305",
            "AES-256-GCM",
        };
        Peer owner;
    };
}