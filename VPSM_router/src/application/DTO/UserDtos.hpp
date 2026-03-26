#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace vpsm::server::application::dto {
    struct LoginDto {
        std::string nickname;
        std::string passwordHash;
    };

    struct LoginResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint64_t> peerId;
        std::optional<std::uint64_t> sessionId;
        std::optional<std::uint64_t> sessionKey;
        std::optional<std::string> error;
    };

    struct CreatePeerDto {
        std::string nickname;
        std::string passwordHash;
    };

    struct CreatePeerResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint64_t> peerId;
        std::optional<std::string> error;
    };

    struct CreateNetworkDto {
        std::uint64_t ownerPeerId;
        std::string name;
        std::string passwordHash;
    };

    struct CreateNetworkResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint64_t> networkId;
        std::optional<std::string> error;
    };

    struct JoinNetworkDto {
        std::uint64_t peerId;
        std::uint64_t networkId;
        std::string passwordHash;
    };

    struct JoinNetworkResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint32_t> vip;
        std::optional<std::string> error;
    };

    struct LeaveNetworkDto {
        std::uint64_t peerId;
        std::uint64_t networkId;
    };

    struct LeaveNetworkResultDto {
        bool ok = false;
        std::uint16_t status = 200;
        std::optional<std::string> error;
    };

    struct CreateNetworkAuthDto {
        std::string name;
        std::string passwordHash;
    };

    struct CreateNetworkAuthResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint64_t> networkId;
        std::optional<std::string> error;
    };

    struct NetworkUserAddDto {
        std::uint64_t networkId;
        std::string passwordHash;
    };

    struct NetworkUserAddResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::optional<std::uint32_t> vip;
        bool alreadyExists = false;
        std::optional<std::string> error;
    };

    struct UserNetworkItemDto {
        std::uint64_t id = 0;
        std::string name;
        std::uint64_t ownerPeerId = 0;
    };

    struct UserNetworkListResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::vector<UserNetworkItemDto> networks;
        std::optional<std::string> error;
    };

    struct NetworkPeerItemDto {
        std::uint64_t peerId = 0;
        std::uint32_t vip = 0;
    };

    struct NetworkPeersListResultDto {
        bool ok = false;
        std::uint16_t status = 400;
        std::vector<NetworkPeerItemDto> peers;
        std::optional<std::string> error;
    };
}
