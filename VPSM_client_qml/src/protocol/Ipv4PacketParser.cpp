#include "Ipv4PacketParser.hpp"

#include <QtEndian>

namespace vpsm::client {
    std::optional<Ipv4PacketInfo> Ipv4PacketParser::parse(const QByteArray& packet) {
        if (packet.size() < 20) return std::nullopt;
        const auto* bytes = reinterpret_cast<const uchar*>(packet.constData());
        if ((bytes[0] >> 4) != 4) return std::nullopt;
        const int headerLength = (bytes[0] & 0x0f) * 4;
        if (headerLength < 20 || headerLength > packet.size()) return std::nullopt;
        const int totalLength = qFromBigEndian<quint16>(bytes + 2);
        if (totalLength < headerLength || totalLength > packet.size()) return std::nullopt;
        return Ipv4PacketInfo{
            .source = qFromBigEndian<quint32>(bytes + 12),
            .destination = qFromBigEndian<quint32>(bytes + 16),
            .protocol = bytes[9],
            .headerLength = headerLength,
            .totalLength = totalLength,
        };
    }

    bool Ipv4PacketParser::belongsToSubnet(quint32 address, quint32 network, int prefixLength) {
        if (prefixLength <= 0 || prefixLength > 32) return false;
        const quint32 mask = prefixLength == 32 ? 0xffffffffu : (0xffffffffu << (32 - prefixLength));
        return (address & mask) == (network & mask);
    }
}