#include "PacketParserV2.hpp"

namespace vpsm::server::application {
    std::optional<domain::OutPacketHeaderV2> PacketParserV2::parseOuter(const domain::PacketIn& pkt) {
        if (!pkt.buf) {
            return std::nullopt;
        }

        if (pkt.size < domain::OUTER_HEADER_V2_SIZE) {
            return std::nullopt;
        }

        if (pkt.buf->size() < pkt.size) {
            return std::nullopt;
        }

        const auto* data = pkt.buf->data();
        std::size_t ptr = 0;

        const auto version = data[ptr++];
        if (version != domain::VERSION_V2) {
            return std::nullopt;
        }

        domain::OutPacketHeaderV2 header{};
        header.packetVersion = version;
        header.sessionId = readU64BE(data + ptr);
        ptr += sizeof(std::uint64_t);
        header.seq = readU64BE(data + ptr);

        return header;
    }

    std::optional<domain::InnerPacketHeaderV2> PacketParserV2::parseInner(
        const std::uint8_t* data,
        std::size_t len,
        std::size_t off
    ) {
        if (data == nullptr) {
            return std::nullopt;
        }

        if (len < off + domain::INNER_HEADER_V2_SIZE) {
            return std::nullopt;
        }

        std::size_t ptr = off;
        const std::uint8_t typeRaw = data[ptr++];

        domain::PacketTypeV2 type;
        switch (typeRaw) {
            case 0: type = domain::PacketTypeV2::DATA; break;
            case 1: type = domain::PacketTypeV2::PROBE; break;
            case 2: type = domain::PacketTypeV2::KEEPALIVE; break;
            default: return std::nullopt;
        }

        domain::InnerPacketHeaderV2 inner{};
        inner.packetType = type;
        inner.vNetworkId = readU32BE(data + ptr);
        ptr += sizeof(std::uint32_t);
        inner.srcVip = readU32BE(data + ptr);
        ptr += sizeof(std::uint32_t);
        inner.dstVip = readU32BE(data + ptr);

        return inner;
    }

    std::uint32_t PacketParserV2::readU32BE(const std::uint8_t* p) {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
               (static_cast<std::uint32_t>(p[1]) << 16) |
               (static_cast<std::uint32_t>(p[2]) << 8)  |
               (static_cast<std::uint32_t>(p[3]));
    }

    std::uint64_t PacketParserV2::readU64BE(const std::uint8_t* p) {
        std::uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | static_cast<std::uint64_t>(p[i]);
        }
        return value;
    }
}
