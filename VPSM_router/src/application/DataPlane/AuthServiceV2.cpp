#include "AuthServiceV2.hpp"

#include "PacketParserV2.hpp"

namespace vpsm::server::application {
    std::optional<domain::AuthResultV2> AuthServiceV2::verifyAndDecrypt(const domain::PacketIn& packet) {
        const auto outer = PacketParserV2::parseOuter(packet);
        if (!outer.has_value()) {
            return std::nullopt;
        }

        const auto sessionIt = sessionStateById_.find(outer->sessionId);
        if (sessionIt == sessionStateById_.end()) {
            return std::nullopt;
        }

        if (outer->seq <= sessionIt->second.highestSeq) {
            return std::nullopt;
        }

        if (!packet.buf || packet.size < domain::OUTER_HEADER_V2_SIZE + domain::INNER_HEADER_V2_SIZE) {
            return std::nullopt;
        }

        const std::size_t innerOffset = domain::OUTER_HEADER_V2_SIZE;
        const auto inner = PacketParserV2::parseInner(packet.buf->data(), packet.size, innerOffset);
        if (!inner.has_value()) {
            return std::nullopt;
        }

        sessionIt->second.highestSeq = outer->seq;

        return domain::AuthResultV2{
            .outer = *outer,
            .inner = *inner,
            .authenticatedPeerId = sessionIt->second.peerId,
        };
    }
}
