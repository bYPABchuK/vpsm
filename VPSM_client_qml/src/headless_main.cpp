#include "application/TunnelController.hpp"
#include "infrastructure/HttpControlPlaneClient.hpp"
#include "infrastructure/UdpTunnelTransport.hpp"
#include "platform/PlatformServicesFactory.hpp"
#include "presentation/AppViewModel.hpp"
#include "presentation/TunnelViewModel.hpp"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

using namespace vpsm::client;

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("vpsm_tunnel_headless"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("VPSM Linux IPv4 TUN client core"));
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList{"router"}, "Router IPv4 address", "address"));
    parser.addOption(QCommandLineOption(QStringList{"control-url"}, "Control Plane URL for automatic login/join", "url"));
    parser.addOption(QCommandLineOption(QStringList{"nickname"}, "Login nickname", "name"));
    parser.addOption(QCommandLineOption(QStringList{"password"}, "Login password hash/value", "password"));
    parser.addOption(QCommandLineOption(QStringList{"network-password"}, "Virtual network password hash/value", "password"));
    parser.addOption(QCommandLineOption(QStringList{"router-port"}, "Router UDP port", "port", "4000"));
    parser.addOption(QCommandLineOption(QStringList{"local-port"}, "Local UDP port", "port", "4001"));
    parser.addOption(QCommandLineOption(QStringList{"session-id"}, "Session ID", "id"));
    parser.addOption(QCommandLineOption(QStringList{"data-key"}, "64-character data-plane key", "hex"));
    parser.addOption(QCommandLineOption(QStringList{"peer-id"}, "Peer ID", "id", "1"));
    parser.addOption(QCommandLineOption(QStringList{"session-key"}, "Control session key", "key", "1"));
    parser.addOption(QCommandLineOption(QStringList{"network-id"}, "Virtual network ID", "id"));
    parser.addOption(QCommandLineOption(QStringList{"address"}, "Local overlay IPv4 address", "address"));
    parser.addOption(QCommandLineOption(QStringList{"network"}, "Overlay network IPv4 address", "address"));
    parser.addOption(QCommandLineOption(QStringList{"prefix"}, "Overlay prefix length", "bits", "24"));
    parser.addOption(QCommandLineOption(QStringList{"mtu"}, "TUN MTU", "bytes", "1400"));
    parser.addOption(QCommandLineOption(QStringList{"interface"}, "Preferred TUN name", "name", "vpsm0"));
    parser.process(app);

    bool ok = false;
    Session session;
    session.peerId = parser.value("peer-id").toULongLong(&ok);
    if (!ok) session.peerId = 0;
    session.sessionId = parser.value("session-id").toULongLong(&ok);
    if (!ok) session.sessionId = 0;
    session.sessionKey = parser.value("session-key").toULongLong(&ok);
    if (!ok) session.sessionKey = 0;
    session.dataPlaneKey = QByteArray::fromHex(parser.value("data-key").toLatin1());

    VirtualNetwork network;
    network.networkId = parser.value("network-id").toUInt(&ok);
    if (!ok) network.networkId = 0;
    network.localAddress = QHostAddress(parser.value("address"));
    network.networkAddress = QHostAddress(parser.value("network"));
    network.prefixLength = parser.value("prefix").toInt();
    network.mtu = parser.value("mtu").toInt();

    RouterEndpoint router;
    router.address = QHostAddress(parser.value("router"));
    router.port = parser.value("router-port").toUShort();
    router.localPort = parser.value("local-port").toUShort();

    auto platform = createPlatformServices();
    auto tun = platform->createTunDevice();
    auto configurator = platform->createNetworkConfigurator();
    UdpTunnelTransport transport;
    TunnelController controller(*tun, *configurator, transport);
    TunnelViewModel tunnelViewModel(controller);
    HttpControlPlaneClient controlPlane;
    AppViewModel appViewModel(controlPlane, tunnelViewModel);
    QObject::connect(&controller, &TunnelController::stateChanged, &app,
        [](TunnelState state) { qInfo() << "tunnel state" << static_cast<int>(state); });
    QObject::connect(&controller, &TunnelController::errorChanged, &app,
        [](const QString& error) { if (!error.isEmpty()) qCritical().noquote() << error; });

    QObject::connect(&appViewModel, &AppViewModel::statusTextChanged, &app,
        [&appViewModel]() { qInfo().noquote() << appViewModel.statusText(); });
    QObject::connect(&appViewModel, &AppViewModel::errorTextChanged, &app,
        [&appViewModel, &app]() {
            if (!appViewModel.errorText().isEmpty()) {
                qCritical().noquote() << appViewModel.errorText();
                QMetaObject::invokeMethod(&app, [&app]() { app.exit(2); }, Qt::QueuedConnection);
            }
        });

    if (!parser.value("control-url").isEmpty()) {
        QObject::connect(&appViewModel, &AppViewModel::serverConnectedChanged, &app,
            [&appViewModel, &parser]() {
                if (!appViewModel.serverConnected()) return;
                appViewModel.login(
                    parser.value("control-url"),
                    parser.value("nickname"),
                    parser.value("password")
                );
            });
        QObject::connect(&appViewModel, &AppViewModel::authenticatedChanged, &app,
            [&appViewModel, &parser, router]() {
                if (!appViewModel.authenticated()) return;
                bool networkIdOk = false;
                const auto networkId = parser.value("network-id").toUInt(&networkIdOk);
                if (!networkIdOk) return;
                appViewModel.connectNetwork(
                    networkId,
                    parser.value("network-password"),
                    router.address.toString(),
                    router.port,
                    router.localPort,
                    parser.value("interface")
                );
            });
        appViewModel.connectServer(parser.value("control-url"));
    } else if (!controller.start(session, network, router, parser.value("interface"))) {
        return 2;
    }
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &controller, &TunnelController::stop);
    return app.exec();
}