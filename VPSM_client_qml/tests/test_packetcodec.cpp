#include "protocol/Ipv4PacketParser.hpp"
#include "protocol/PacketCodecV2.hpp"
#include "protocol/ReplayWindow.hpp"
#include "TestFakes.hpp"

#include <QtTest/QtTest>

using namespace vpsm::client;

class PacketCodecTest final : public QObject {
    Q_OBJECT
private slots:
    void clientPacketRoundTripAndDirectionSeparation();
    void tamperedPacketIsRejected();
    void replayWindowAllowsReorderingOnce();
    void ipv4ParserValidatesPacketAndSubnet();
    void matchesIndependentOpenSslTestVector();
};

void PacketCodecTest::clientPacketRoundTripAndDirectionSeparation() {
    const auto session = test::validSession();
    const auto encoded = PacketCodecV2::encodeClientPacket(
        session, 7, PacketType::Data, 10,
        QHostAddress("10.240.1.1").toIPv4Address(),
        QHostAddress("10.240.1.2").toIPv4Address(),
        QByteArray("payload")
    );
    QVERIFY(encoded.has_value());
    const auto decoded = PacketCodecV2::decodeClientPacket(session, *encoded);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->sequence, quint64(7));
    QCOMPARE(decoded->networkId, quint32(10));
    QCOMPARE(decoded->payload, QByteArray("payload"));
    QVERIFY(!PacketCodecV2::decodeRouterPacket(session, *encoded).has_value());
}

void PacketCodecTest::tamperedPacketIsRejected() {
    const auto session = test::validSession();
    auto encoded = *PacketCodecV2::encodeRouterPacket(
        session, 1, PacketType::Keepalive, 10,
        QHostAddress("10.240.1.1").toIPv4Address(),
        QHostAddress("10.240.1.1").toIPv4Address(), {}
    );
    encoded[17] = static_cast<char>(encoded[17] ^ 1);
    QVERIFY(!PacketCodecV2::decodeRouterPacket(session, encoded).has_value());
}

void PacketCodecTest::replayWindowAllowsReorderingOnce() {
    ReplayWindow replay;
    QVERIFY(replay.accept(100));
    QVERIFY(replay.accept(98));
    QVERIFY(!replay.accept(98));
    QVERIFY(!replay.accept(35));
    replay.reset();
    QVERIFY(replay.accept(1));
}

void PacketCodecTest::ipv4ParserValidatesPacketAndSubnet() {
    const auto source = QHostAddress("10.240.1.1").toIPv4Address();
    const auto destination = QHostAddress("10.240.1.2").toIPv4Address();
    const auto packet = test::ipv4Packet(source, destination, QByteArray("abc"));
    const auto parsed = Ipv4PacketParser::parse(packet);
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->source, source);
    QCOMPARE(parsed->destination, destination);
    QCOMPARE(parsed->totalLength, packet.size());
    QVERIFY(Ipv4PacketParser::belongsToSubnet(destination,
        QHostAddress("10.240.1.0").toIPv4Address(), 24));
    QVERIFY(!Ipv4PacketParser::belongsToSubnet(
        QHostAddress("10.240.2.1").toIPv4Address(),
        QHostAddress("10.240.1.0").toIPv4Address(), 24));
    QVERIFY(!Ipv4PacketParser::parse(QByteArray(19, '\0')).has_value());
}

void PacketCodecTest::matchesIndependentOpenSslTestVector() {
    Session session;
    session.peerId = 1;
    session.sessionId = 0x0102030405060708ull;
    session.sessionKey = 1;
    for (int i = 0; i < 32; ++i) session.dataPlaneKey.append(static_cast<char>(i));
    const auto encoded = PacketCodecV2::encodeClientPacket(
        session,
        0x1112131415161718ull,
        PacketType::Data,
        0x0a0b0c0d,
        0x0af00101,
        0x0af00102,
        QByteArray("ssh")
    );
    QVERIFY(encoded.has_value());
    QCOMPARE(encoded->toHex(), QByteArray(
        "0201020304050607081112131415161718"
        "218ba6c072d689271d1cc06b893fcbf2"
        "fba2d81514fdc2531a2570b3cc0c06c6"));
}

QTEST_MAIN(PacketCodecTest)
#include "test_packetcodec.moc"