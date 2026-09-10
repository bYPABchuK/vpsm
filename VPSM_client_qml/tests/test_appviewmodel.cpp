#include "presentation/AppViewModel.hpp"
#include "protocol/PacketCodecV2.hpp"
#include "TestFakes.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

using namespace vpsm::client;

class AppViewModelTest final : public QObject {
    Q_OBJECT
private slots:
    void serverUserNetworkSelectionDrivesTunnelState();
    void protectedOperationsRequireServerAndUserState();
    void errorsAndLogoutAreExposed();
    void tunnelSetupFailureKeepsPrimaryErrorState();
    void networkManagementCommandsAreForwardedAndRefreshed();
};

void AppViewModelTest::serverUserNetworkSelectionDrivesTunnelState() {
    test::FakeControlPlaneClient control;
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    TunnelViewModel tunnel(controller);
    AppViewModel app(control, tunnel);

    app.connectServer(QStringLiteral("http://router:8080"));
    QCOMPARE(control.probeCalls, 1);
    control.completeServerProbe();
    QVERIFY(app.serverConnected());

    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    QCOMPARE(control.loginCalls, 1);
    QCOMPARE(app.statusText(), QStringLiteral("Authenticating"));
    control.completeLogin(test::validSession());
    QVERIFY(app.authenticated());
    QCOMPARE(app.statusText(), QStringLiteral("Authenticated"));

    app.refreshNetworks();
    QCOMPARE(control.listCalls, 1);
    control.completeList({test::validNetwork()});
    QCOMPARE(app.networks().size(), 1);
    app.connectAvailableNetwork(10, QStringLiteral("127.0.0.1"), 4000, 4001);
    QCOMPARE(control.joinCalls, 0);
    QCOMPARE(tunnel.stateText(), QStringLiteral("Registering endpoint"));

    const auto local = test::validNetwork().localAddress.toIPv4Address();
    transport.inject(*PacketCodecV2::encodeRouterPacket(
        test::validSession(), 1, PacketType::Keepalive, 10, local, local, {}));
    QVERIFY(tunnel.connected());
    QCOMPARE(app.statusText(), QStringLiteral("Connected"));
}

void AppViewModelTest::protectedOperationsRequireServerAndUserState() {
    test::FakeControlPlaneClient control;
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    TunnelViewModel tunnel(controller);
    AppViewModel app(control, tunnel);

    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    QCOMPARE(control.loginCalls, 0);
    QCOMPARE(app.errorText(), QStringLiteral("Connect to the server first"));

    app.connectServer(QStringLiteral("http://router:8080"));
    control.completeServerProbe();
    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    control.completeLogin(test::validSession());
    app.refreshNetworks();
    control.completeList({test::validNetwork()});

    app.connectAvailableNetwork(999, QStringLiteral("127.0.0.1"), 4000, 4001);
    QCOMPARE(app.errorText(), QStringLiteral("Select an available network"));
    QCOMPARE(tunnel.stateText(), QStringLiteral("Disconnected"));
}

void AppViewModelTest::errorsAndLogoutAreExposed() {
    test::FakeControlPlaneClient control;
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    TunnelViewModel tunnel(controller);
    AppViewModel app(control, tunnel);

    app.connectNetwork(10, {}, QStringLiteral("127.0.0.1"), 4000, 4001);
    QVERIFY(app.errorText().contains(QStringLiteral("Login")));
    app.connectServer(QStringLiteral("http://router:8080"));
    control.completeServerProbe();
    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    control.completeLogin({}, QStringLiteral("invalid_password"));
    QVERIFY(!app.authenticated());
    QCOMPARE(app.errorText(), QStringLiteral("invalid_password"));

    control.completeLogin(test::validSession());
    QVERIFY(app.authenticated());
    app.logout();
    QCOMPARE(control.logoutCalls, 1);
    control.completeLogout();
    QVERIFY(!app.authenticated());
    QCOMPARE(app.statusText(), QStringLiteral("Logged out"));
}

void AppViewModelTest::tunnelSetupFailureKeepsPrimaryErrorState() {
    test::FakeControlPlaneClient control;
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    tun.createResult = false;
    TunnelController controller(tun, config, transport);
    TunnelViewModel tunnel(controller);
    AppViewModel app(control, tunnel);
    app.connectServer(QStringLiteral("http://router:8080"));
    control.completeServerProbe();
    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    control.completeLogin(test::validSession());
    app.refreshNetworks();
    control.completeList({test::validNetwork()});

    app.connectAvailableNetwork(10, QStringLiteral("127.0.0.1"), 4000, 4001);

    QCOMPARE(app.statusText(), QStringLiteral("Tunnel error"));
    QVERIFY(app.errorText().contains(QStringLiteral("fake TUN")));
}

void AppViewModelTest::networkManagementCommandsAreForwardedAndRefreshed() {
    test::FakeControlPlaneClient control;
    test::FakeTunDevice tun;
    test::FakeNetworkConfigurator config;
    test::FakeTunnelTransport transport;
    TunnelController controller(tun, config, transport);
    TunnelViewModel tunnel(controller);
    AppViewModel app(control, tunnel);
    app.connectServer(QStringLiteral("http://router:8080"));
    control.completeServerProbe();
    app.login(QStringLiteral("http://router:8080"), QStringLiteral("alice"), QStringLiteral("pw"));
    control.completeLogin(test::validSession());

    app.createNetwork(QStringLiteral("team"), QStringLiteral("net-pw"));
    QCOMPARE(control.createCalls, 1);
    control.completeCreate(20);
    QCOMPARE(control.peerListCalls, 1);

    app.joinNetwork(QStringLiteral("team"), QStringLiteral("net-pw"));
    QCOMPARE(control.joinCalls, 1);
    QCOMPARE(control.lastNickname, QStringLiteral("team"));
    app.removePeer(20, QStringLiteral("42"));
    QCOMPARE(control.leaveCalls, 1);
    QCOMPARE(control.lastPeerId, 42u);
    app.leaveNetwork(20);
    QCOMPARE(control.leaveCalls, 2);
    QCOMPARE(control.lastPeerId, test::validSession().peerId);
    app.deleteNetwork(20);
    QCOMPARE(control.deleteCalls, 1);
}

QTEST_MAIN(AppViewModelTest)
#include "test_appviewmodel.moc"