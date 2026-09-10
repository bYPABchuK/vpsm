#include "presentation/TunnelViewModel.hpp"
#include "protocol/PacketCodecV2.hpp"
#include "TestFakes.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

using namespace vpsm::client;

class TunnelViewModelTest final : public QObject {
    Q_OBJECT
private slots:
    void exposesStateNetworkAndStatistics();
    void exposesSetupError();
};

void TunnelViewModelTest::exposesStateNetworkAndStatistics() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    TunnelViewModel viewModel(controller);
    QSignalSpy stateSpy(&viewModel, &TunnelViewModel::stateChanged);
    QSignalSpy statsSpy(&viewModel, &TunnelViewModel::statisticsChanged);

    QVERIFY(viewModel.connectTunnel(test::validSession(), test::validNetwork(), test::validRouter()));
    QVERIFY(viewModel.busy());
    QCOMPARE(viewModel.stateText(), QStringLiteral("Registering endpoint"));

    const auto local = test::validNetwork().localAddress.toIPv4Address();
    transport.inject(*PacketCodecV2::encodeRouterPacket(
        test::validSession(), 1, PacketType::Keepalive, 10, local, local, {}));
    QVERIFY(viewModel.connected());
    QCOMPARE(viewModel.localAddress(), QStringLiteral("10.240.1.1"));
    QCOMPARE(viewModel.interfaceName(), QStringLiteral("vpsm-test0"));

    tun.inject(test::ipv4Packet(local, QHostAddress("10.240.1.2").toIPv4Address()));
    QCOMPARE(viewModel.txPackets(), qulonglong(1));
    QVERIFY(statsSpy.count() >= 2);
    QVERIFY(stateSpy.count() >= 4);

    viewModel.disconnectTunnel();
    QVERIFY(!viewModel.connected());
    QCOMPARE(viewModel.stateText(), QStringLiteral("Disconnected"));
}

void TunnelViewModelTest::exposesSetupError() {
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    tun.createResult = false;
    TunnelController controller(tun, config, transport);
    TunnelViewModel viewModel(controller);
    QSignalSpy errorSpy(&viewModel, &TunnelViewModel::errorTextChanged);

    QVERIFY(!viewModel.connectTunnel(test::validSession(), test::validNetwork(), test::validRouter()));
    QCOMPARE(viewModel.stateText(), QStringLiteral("Error"));
    QVERIFY(viewModel.errorText().contains(QStringLiteral("fake TUN")));
    QVERIFY(errorSpy.count() >= 1);
}

QTEST_MAIN(TunnelViewModelTest)
#include "test_tunnelviewmodel.moc"