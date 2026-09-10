#pragma once

#include <cstddef>
#include <cstdint>

namespace vpsm::client::helper_protocol {
    constexpr std::uint32_t Magic = 0x5650534Du; // VPSM
    constexpr std::uint16_t Version = 1;
    constexpr std::size_t InterfaceNameSize = 16;
    constexpr std::size_t ErrorSize = 256;

    enum class Command : std::uint16_t {
        CreateTun = 1,
        Configure = 2,
        Cleanup = 3,
        Shutdown = 4,
    };

    struct Request {
        std::uint32_t magic = Magic;
        std::uint16_t version = Version;
        Command command = Command::CreateTun;
        char interfaceName[InterfaceNameSize]{};
        std::uint32_t localAddress = 0;
        std::uint32_t networkAddress = 0;
        std::uint16_t mtu = 0;
        std::uint8_t prefixLength = 0;
        std::uint8_t reserved = 0;
    };

    struct Response {
        std::uint32_t magic = Magic;
        std::uint16_t version = Version;
        std::int16_t status = 0;
        char interfaceName[InterfaceNameSize]{};
        char error[ErrorSize]{};
    };

    bool sendRequest(int socket, const Request& request);
    bool receiveRequest(int socket, Request& request);
    bool sendResponse(int socket, const Response& response, int descriptor = -1);
    bool receiveResponse(int socket, Response& response, int& descriptor);
}