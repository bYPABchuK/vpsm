#pragma once

#include <cstdint>
#include <variant>

namespace vpsm::server::port {
    enum class UserServiceError {
        PeerNotFound,
        NetworkNotFound,
        NetworkNameAlreadyExists,
        InvalidPassword,
        AlreadyMember,
        NotMember,
        Forbidden,
        CreateFailed,
        DeleteFailed,
        InternalError,
    };

    struct ActionSuccess {};

    struct CreatePeerSuccess {
        std::uint64_t peerId = 0;
    };

    struct CreateNetworkSuccess {
        std::uint64_t networkId = 0;
    };

    struct JoinNetworkSuccess {
        std::uint32_t vip = 0;
        std::uint32_t networkAddress = 0;
        std::uint8_t prefixLength = 0;
        std::uint16_t mtu = 0;
        bool alreadyExists = false;
        std::uint64_t networkId = 0;
    };

    using CreatePeerResult = std::variant<CreatePeerSuccess, UserServiceError>;
    using CreateNetworkResult = std::variant<CreateNetworkSuccess, UserServiceError>;
    using JoinNetworkResult = std::variant<JoinNetworkSuccess, UserServiceError>;
    using ActionResult = std::variant<ActionSuccess, UserServiceError>;
}