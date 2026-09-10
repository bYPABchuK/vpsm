#include "application/TunnelController.hpp"
#include "protocol/PacketCodecV2.hpp"
#include "TestFakes.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

using namespace vpsm::client;

class TunnelControllerTest final : public QObject {
    Q_OBJECT
private slots:
    void startWaitsForAuthenticatedKeepalive();
    void tunPacketIsEncryptedAndInboundPacketIsWritten();
    void spoofedAndReplayedPacketsAreDropped();
    void setupFailureRollsBackResources();
    void registrationTimeoutFailsAndCleansUp();
    void inboundPacketsFromAllAllowedNetworksAreAccepted();
};

namespace {
void completeRegistration(TunnelController& controller, test::FakeTunnelTransport& transport) {
    const auto session = test::validSession();
    const auto network = test::validNetwork();
    const auto local = network.localAddress.toIPv4Address();
    const auto response = PacketCodecV2::encodeRouterPacket(
        session, 1, PacketType::Keepalive, network.networkId, local, local, {}
    );
    QVERIFY(response.has_value());
    transport.inject(*response);
    QCOMPARE(controller.state(), TunnelState::Connected);
}
}

void TunnelControllerTest::startWaitsForAuthenticatedKeepalive() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    QSignalSpy stateSpy(&controller, &TunnelController::stateChanged);

    QVERIFY(controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    QCOMPARE(controller.state(), TunnelState::RegisteringEndpoint);
    QCOMPARE(transport.sentDatagrams.size(), 1);
    const auto registration = PacketCodecV2::decodeClientPacket(
        test::validSession(), transport.sentDatagrams.first());
    QVERIFY(registration.has_value());
    QCOMPARE(registration->type, PacketType::Keepalive);
    QCOMPARE(registration->sourceVip, test::validNetwork().localAddress.toIPv4Address());

    completeRegistration(controller, transport);
    QVERIFY(stateSpy.count() >= 4);
    controller.stop();
    QCOMPARE(controller.state(), TunnelState::Disconnected);
    QCOMPARE(config.removeCalls, 1);
    QVERIFY(tun.closed);
    QCOMPARE(transport.closeCalls, 1);
}

void TunnelControllerTest::tunPacketIsEncryptedAndInboundPacketIsWritten() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    QVERIFY(controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    completeRegistration(controller, transport);

    const auto local = test::validNetwork().localAddress.toIPv4Address();
    const auto remote = QHostAddress("10.240.1.2").toIPv4Address();
    const auto outbound = test::ipv4Packet(local, remote, QByteArray("ssh"));
    tun.inject(outbound);
    QCOMPARE(transport.sentDatagrams.size(), 2);
    const auto decodedOutbound = PacketCodecV2::decodeClientPacket(
        test::validSession(), transport.sentDatagrams.last());
    QVERIFY(decodedOutbound.has_value());
    QCOMPARE(decodedOutbound->type, PacketType::Data);
    QCOMPARE(decodedOutbound->payload, outbound);
    QCOMPARE(controller.statistics().txPackets, quint64(1));

    const auto inbound = test::ipv4Packet(remote, local, QByteArray("reply"));
    const auto datagram = PacketCodecV2::encodeRouterPacket(
        test::validSession(), 2, PacketType::Data, 10, remote, local, inbound);
    QVERIFY(datagram.has_value());
    transport.inject(*datagram);
    QCOMPARE(tun.writtenPackets.size(), 1);
    QCOMPARE(tun.writtenPackets.first(), inbound);
    QCOMPARE(controller.statistics().rxPackets, quint64(1));
}

void TunnelControllerTest::spoofedAndReplayedPacketsAreDropped() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    QVERIFY(controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    completeRegistration(controller, transport);

    const auto local = test::validNetwork().localAddress.toIPv4Address();
    const auto remote = QHostAddress("10.240.1.2").toIPv4Address();
    tun.inject(test::ipv4Packet(QHostAddress("10.240.1.99").toIPv4Address(), remote));
    QCOMPARE(transport.sentDatagrams.size(), 1);

    const auto inner = test::ipv4Packet(remote, local);
    const auto packet = *PacketCodecV2::encodeRouterPacket(
        test::validSession(), 2, PacketType::Data, 10, remote, local, inner);
    transport.inject(packet);
    transport.inject(packet);
    QCOMPARE(tun.writtenPackets.size(), 1);
    QVERIFY(controller.statistics().droppedPackets >= 2);

    const auto outside = QHostAddress("10.241.1.2").toIPv4Address();
    const auto outsideInner = test::ipv4Packet(outside, local);
    transport.inject(*PacketCodecV2::encodeRouterPacket(
        test::validSession(), 3, PacketType::Data, 10, outside, local, outsideInner));
    QCOMPARE(tun.writtenPackets.size(), 1);
}

void TunnelControllerTest::setupFailureRollsBackResources() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    transport.openResult = false;
    TunnelController controller(tun, config, transport);

    QVERIFY(!controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    QCOMPARE(controller.state(), TunnelState::Error);
    QCOMPARE(config.removeCalls, 1);
    QVERIFY(tun.closed);
    QVERIFY(!controller.errorText().isEmpty());
}

void TunnelControllerTest::registrationTimeoutFailsAndCleansUp() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    controller.setRegistrationTimeout(20);
    QVERIFY(controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    QTRY_COMPARE_WITH_TIMEOUT(controller.state(), TunnelState::Error, 200);
    QVERIFY(tun.closed);
    QCOMPARE(config.removeCalls, 1);
    QCOMPARE(transport.closeCalls, 1);
}

void TunnelControllerTest::inboundPacketsFromAllAllowedNetworksAreAccepted() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    controller.setAllowedNetworkIds(QSet<quint32>{10, 20});
    const auto remote = QHostAddress("10.240.1.2").toIPv4Address();
    controller.setDestinationNetworks(QHash<quint32, quint32>{{remote, 20}});
    QVERIFY(controller.start(test::validSession(), test::validNetwork(), test::validRouter()));
    completeRegistration(controller, transport);

    const auto local = test::validNetwork().localAddress.toIPv4Address();
    tun.inject(test::ipv4Packet(local, remote));
    const auto outbound = PacketCodecV2::decodeClientPacket(test::validSession(), transport.sentDatagrams.last());
    QVERIFY(outbound.has_value());
    QCOMPARE(outbound->networkId, 20u);

    const auto inner = test::ipv4Packet(remote, local);
    transport.inject(*PacketCodecV2::encodeRouterPacket(
        test::validSession(), 3, PacketType::Data, 20, remote, local, inner));
    QCOMPARE(tun.writtenPackets.size(), 1);

    transport.inject(*PacketCodecV2::encodeRouterPacket(
        test::validSession(), 4, PacketType::Data, 30, remote, local, inner));
    QCOMPARE(tun.writtenPackets.size(), 1);
}

QTEST_MAIN(TunnelControllerTest)
#include "test_tunnelcontroller.moc"