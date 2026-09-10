#include "../../src/application/ControlPlane/SessionStore.hpp"
#include "../../src/application/DataPlane/AuthServiceV2.hpp"
#include "../../src/application/DataPlane/PacketCryptoV2.hpp"
#include "../../src/application/ControlPlane/JsonLoginResponseEncoder.hpp"

#include <boost/json/parse.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace {
    using vpsm::server::application::AuthServiceV2;
    using vpsm::server::application::PacketCryptoV2;
    using vpsm::server::application::SessionStore;
    using vpsm::server::domain::PacketIn;

    void putU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
        for (int i = 3; i >= 0; --i) out.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
    }

    std::vector<std::uint8_t> makePlaintext(
        std::uint8_t packetType = 0,
        std::uint32_t network = 10,
        std::uint32_t src = 100,
        std::uint32_t dst = 200,
        std::vector<std::uint8_t> payload = {'h', 'i'}
    ) {
        std::vector<std::uint8_t> out{packetType};
        putU32(out, network);
        putU32(out, src);
        putU32(out, dst);
        out.insert(out.end(), payload.begin(), payload.end());
        return out;
    }

    std::vector<std::uint8_t> makeEncryptedPacket(
        const SessionStore::SessionData& session,
        std::uint64_t sequence,
        const std::vector<std::uint8_t>& plaintext = makePlaintext()
    ) {
        const vpsm::server::domain::OutPacketHeaderV2 outer{
            .packetVersion = 2,
            .sessionId = session.sessionId,
            .seq = sequence,
        };
        const auto encrypted = PacketCryptoV2::encrypt(
            outer, plaintext, session.dataPlaneKey,
            PacketCryptoV2::Direction::ClientToRouter
        );
        EXPECT_TRUE(encrypted.has_value());
        return encrypted.value_or(std::vector<std::uint8_t>{});
    }

    PacketIn packetIn(std::vector<std::uint8_t> raw) {
        auto buffer = std::make_shared<std::vector<std::uint8_t>>(std::move(raw));
        return PacketIn{
            .buf = buffer,
            .size = buffer->size(),
            .type = vpsm::server::domain::UDP,
        };
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_ValidAeadPacket_ReturnsIdentityAndPlaintext) {
        SessionStore sessions;
        const auto session = sessions.createSession(42);
        AuthServiceV2 auth(sessions);

        const auto result = auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 1)));

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->outer.sessionId, session.sessionId);
        EXPECT_EQ(result->outer.seq, 1u);
        EXPECT_EQ(result->inner.vNetworkId, 10u);
        ASSERT_TRUE(result->authenticatedPeerId.has_value());
        EXPECT_EQ(*result->authenticatedPeerId, 42u);
        ASSERT_TRUE(result->plaintextInnerAndPayload);
        EXPECT_EQ(*result->plaintextInnerAndPayload, makePlaintext());
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_UnknownSession_DropsPacket) {
        SessionStore sourceSessions;
        const auto foreignSession = sourceSessions.createSession(42);
        SessionStore routerSessions;
        AuthServiceV2 auth(routerSessions);

        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(foreignSession, 1))).has_value());
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_ModifiedCiphertext_DoesNotAdvanceReplayWindow) {
        SessionStore sessions;
        const auto session = sessions.createSession(42);
        AuthServiceV2 auth(sessions);
        auto forged = makeEncryptedPacket(session, 500);
        forged[vpsm::server::domain::OUTER_HEADER_V2_SIZE] ^= 0x80;

        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(std::move(forged))).has_value());
        EXPECT_TRUE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 1))).has_value());
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_ReplayAndModifiedTag_Drop) {
        SessionStore sessions;
        const auto session = sessions.createSession(42);
        AuthServiceV2 auth(sessions);
        const auto valid = makeEncryptedPacket(session, 10);

        EXPECT_TRUE(auth.verifyAndDecrypt(packetIn(valid)).has_value());
        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(valid)).has_value());

        auto modifiedTag = makeEncryptedPacket(session, 11);
        modifiedTag.back() ^= 1;
        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(std::move(modifiedTag))).has_value());
    }

    TEST(AuthServiceV2Test, verifyAndDecrypt_OutOfOrderWithinWindow_AcceptsOnce) {
        SessionStore sessions;
        const auto session = sessions.createSession(42);
        AuthServiceV2 auth(sessions);

        EXPECT_TRUE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 100))).has_value());
        EXPECT_TRUE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 98))).has_value());
        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 98))).has_value());
        EXPECT_FALSE(auth.verifyAndDecrypt(packetIn(makeEncryptedPacket(session, 35))).has_value());
    }

    TEST(AuthServiceV2Test, expiredAndRevokedSessions_AreRejected) {
        SessionStore expiredStore(std::chrono::seconds(0));
        const auto expired = expiredStore.createSession(1);
        AuthServiceV2 expiredAuth(expiredStore);
        EXPECT_FALSE(expiredAuth.verifyAndDecrypt(packetIn(makeEncryptedPacket(expired, 1))).has_value());

        SessionStore revokedStore;
        const auto revoked = revokedStore.createSession(2);
        ASSERT_TRUE(revokedStore.revoke(revoked.sessionId));
        AuthServiceV2 revokedAuth(revokedStore);
        EXPECT_FALSE(revokedAuth.verifyAndDecrypt(packetIn(makeEncryptedPacket(revoked, 1))).has_value());
    }

    TEST(AuthServiceV2Test, encryptForSession_UsesRouterDirectionAndOutboundSequence) {
        SessionStore sessions;
        const auto destination = sessions.createSession(9);
        AuthServiceV2 auth(sessions);

        const auto packet = auth.encryptForSession(destination.sessionId, makePlaintext());
        ASSERT_TRUE(packet.has_value());
        const auto decrypted = PacketCryptoV2::decrypt(
            (*packet)->data(), (*packet)->size(), destination.dataPlaneKey,
            PacketCryptoV2::Direction::RouterToClient
        );
        ASSERT_TRUE(decrypted.has_value());
        EXPECT_EQ(*decrypted, makePlaintext());
        EXPECT_FALSE(PacketCryptoV2::decrypt(
            (*packet)->data(), (*packet)->size(), destination.dataPlaneKey,
            PacketCryptoV2::Direction::ClientToRouter
        ).has_value());
    }

    TEST(SessionStoreTest, keyHexRoundTripAndControlAuthentication) {
        SessionStore sessions;
        const auto session = sessions.createSession(77);
        const auto encoded = SessionStore::keyToHex(session.dataPlaneKey);
        ASSERT_EQ(encoded.size(), 64u);
        ASSERT_TRUE(SessionStore::keyFromHex(encoded).has_value());
        EXPECT_EQ(*SessionStore::keyFromHex(encoded), session.dataPlaneKey);
        EXPECT_EQ(sessions.authenticate(session.sessionId, session.sessionKey), 77u);
        EXPECT_FALSE(sessions.authenticate(session.sessionId, session.sessionKey + 1).has_value());
    }

    TEST(JsonLoginResponseEncoderTest, CredentialsUseLosslessStringsAndIncludeDataPlaneKey) {
        vpsm::server::application::JsonLoginResponseEncoder encoder;
        const std::string key(64, 'a');
        const auto response = encoder.encode(vpsm::server::application::dto::LoginResultDto{
            .ok = true,
            .status = 200,
            .peerId = 42,
            .sessionId = std::uint64_t{1} << 63,
            .sessionKey = (std::uint64_t{1} << 63) + 1,
            .dataPlaneKey = key,
        });
        const auto body = boost::json::parse(
            std::string(response.body.begin(), response.body.end())
        ).as_object();
        EXPECT_TRUE(body.at("peerId").is_string());
        EXPECT_EQ(body.at("sessionId").as_string(), "9223372036854775808");
        EXPECT_EQ(body.at("sessionKey").as_string(), "9223372036854775809");
        EXPECT_EQ(body.at("dataPlaneKey").as_string(), key);
    }
}