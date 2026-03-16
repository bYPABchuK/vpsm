#include "PacketParser.hpp"
#include <optional>

namespace vpsm::server::application {
    std::optional<domain::PacketHeader> PacketParser::parse(const domain::PacketIn& pkt) {
        if (!pkt.buf) {
            return std::nullopt;
        }

        if (pkt.size < domain::HEADER_SIZE) {
            return std::nullopt;
        }

        if (pkt.buf->size() < pkt.size) {
            return std::nullopt;
        }

        return readHeader(pkt.buf->data(), pkt.size, 0);
    }

    std::optional<domain::PacketHeader> PacketParser::readHeader(
        const std::uint8_t* data,
        std::size_t len,
        std::size_t off
    ) {
        if (data == nullptr) {
            return std::nullopt;
        }

        if (len < off + domain::HEADER_SIZE) {
            return std::nullopt;
        }

        std::size_t ptr = off;

        const std::uint8_t version = data[ptr++];
        if (version != domain::VERSION) {
            return std::nullopt;
        }

        const std::uint8_t typeRaw = data[ptr++];
        domain::PacketType type;
        switch (typeRaw) {
            case 0:
                type = domain::PacketType::DATA;
                break;
            case 1:
                type = domain::PacketType::PROBE;
                break;
            case 2:
                type = domain::PacketType::KEEPALIVE;
                break;
            default:
                return std::nullopt;
        }

        const std::uint32_t vNetworkId = readU32BE(data + ptr);
        ptr += sizeof(vNetworkId);

        const std::uint32_t srcVip = readU32BE(data + ptr);
        ptr += sizeof(srcVip);

        const std::uint32_t dstVip = readU32BE(data + ptr);
        ptr += sizeof(dstVip);

        const std::uint64_t seq = readU64BE(data + ptr);
        ptr += sizeof(seq);

        const std::uint32_t keyId = readU32BE(data + ptr);

        domain::PacketHeader header;
        header.packetVersion = version;
        header.packetType = type;
        header.vNetworkId = vNetworkId;
        header.srcVip = srcVip;
        header.dstVip = dstVip;
        header.seq = seq;
        header.keyId = keyId;

        return header;
    }

    std::uint32_t PacketParser::readU32BE(const std::uint8_t* p) {
        return (static_cast<std::uint32_t>(p[0]) << 24) |
               (static_cast<std::uint32_t>(p[1]) << 16) |
               (static_cast<std::uint32_t>(p[2]) << 8)  |
               (static_cast<std::uint32_t>(p[3]));
    }

    std::uint64_t PacketParser::readU64BE(const std::uint8_t* p) {
        std::uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | static_cast<std::uint64_t>(p[i]);
        }
        return value;
    }

}