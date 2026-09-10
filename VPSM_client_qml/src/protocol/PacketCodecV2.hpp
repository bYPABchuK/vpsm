#pragma once

#include "domain/TunnelTypes.hpp"

#include <QByteArray>
#include <optional>

namespace vpsm::client {
    enum class PacketType : quint8 {
        Data = 0,
        Probe = 1,
        Keepalive = 2,
    };

    struct DecodedPacket {
        quint64 sequence = 0;
        PacketType type = PacketType::Data;
        quint32 networkId = 0;
        quint32 sourceVip = 0;
        quint32 destinationVip = 0;
        QByteArray payload;
    };

    class PacketCodecV2 {
    public:
        static constexpr int MaximumDatagramSize = 2048;
        static constexpr int MaximumPayloadSize = 2002;

        static std::optional<QByteArray> encodeClientPacket(
            const Session& session,
            quint64 sequence,
            PacketType type,
            quint32 networkId,
            quint32 sourceVip,
            quint32 destinationVip,
            const QByteArray& payload
        );

        static std::optional<DecodedPacket> decodeRouterPacket(
            const Session& session,
            const QByteArray& datagram
        );

        static std::optional<DecodedPacket> decodeClientPacket(
            const Session& session,
            const QByteArray& datagram
        );

        static std::optional<QByteArray> encodeRouterPacket(
            const Session& session,
            quint64 sequence,
            PacketType type,
            quint32 networkId,
            quint32 sourceVip,
            quint32 destinationVip,
            const QByteArray& payload
        );

    private:
        enum class Direction : quint32 {
            ClientToRouter = 1,
            RouterToClient = 2,
        };

        static std::optional<QByteArray> encode(
            const Session& session,
            quint64 sequence,
            PacketType type,
            quint32 networkId,
            quint32 sourceVip,
            quint32 destinationVip,
            const QByteArray& payload,
            Direction direction
        );
        static std::optional<DecodedPacket> decode(
            const Session& session,
            const QByteArray& datagram,
            Direction direction
        );
    };
}