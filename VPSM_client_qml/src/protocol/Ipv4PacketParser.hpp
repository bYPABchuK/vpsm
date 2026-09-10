#pragma once

#include <QByteArray>
#include <optional>

namespace vpsm::client {
    struct Ipv4PacketInfo {
        quint32 source = 0;
        quint32 destination = 0;
        quint8 protocol = 0;
        int headerLength = 0;
        int totalLength = 0;
    };

    class Ipv4PacketParser {
    public:
        static std::optional<Ipv4PacketInfo> parse(const QByteArray& packet);
        static bool belongsToSubnet(quint32 address, quint32 network, int prefixLength);
    };
}