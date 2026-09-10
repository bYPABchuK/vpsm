#include "AuthServiceV2.hpp"

#include "PacketCryptoV2.hpp"
#include "PacketParserV2.hpp"

#include <memory>

namespace vpsm::server::application {
    std::optional<domain::AuthResultV2> AuthServiceV2::verifyAndDecrypt(const domain::PacketIn& packet) {
        const auto outer = PacketParserV2::parseOuter(packet);
        if (!outer || !packet.buf) return std::nullopt;

        const auto session = sessionStore_.beginDataPlanePacket(outer->sessionId);
        if (!session) return std::nullopt;

        auto plaintext = PacketCryptoV2::decrypt(
            packet.buf->data(), packet.size, session->key,
            PacketCryptoV2::Direction::ClientToRouter
        );
        if (!plaintext) return std::nullopt;

        const auto inner = PacketParserV2::parseInner(plaintext->data(), plaintext->size());
        if (!inner) return std::nullopt;

        if (!sessionStore_.commitSequence(outer->sessionId, outer->seq)) return std::nullopt;

        return domain::AuthResultV2{
            .outer = *outer,
            .inner = *inner,
            .authenticatedPeerId = session->peerId,
            .plaintextInnerAndPayload = std::make_shared<std::vector<std::uint8_t>>(std::move(*plaintext)),
        };
    }

    std::optional<domain::buffer> AuthServiceV2::encryptForSession(
        std::uint64_t sessionId,
        const std::vector<std::uint8_t>& plaintextInnerAndPayload
    ) {
        const auto session = sessionStore_.beginDataPlanePacket(sessionId);
        const auto sequence = sessionStore_.nextOutboundSequence(sessionId);
        if (!session || !sequence) return std::nullopt;

        const domain::OutPacketHeaderV2 outer{
            .packetVersion = domain::VERSION_V2,
            .sessionId = sessionId,
            .seq = *sequence,
        };
        auto encrypted = PacketCryptoV2::encrypt(
            outer, plaintextInnerAndPayload, session->key,
            PacketCryptoV2::Direction::RouterToClient
        );
        if (!encrypted) return std::nullopt;
        return std::make_shared<std::vector<std::uint8_t>>(std::move(*encrypted));
    }
}