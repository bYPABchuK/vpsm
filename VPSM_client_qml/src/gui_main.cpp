#include "application/TunnelController.hpp"
#include "infrastructure/HttpControlPlaneClient.hpp"
#include "infrastructure/UdpTunnelTransport.hpp"
#include "platform/PlatformServicesFactory.hpp"
#include "presentation/AppViewModel.hpp"
#include "presentation/TunnelViewModel.hpp"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QUrl>
#include <QVariant>

using namespace vpsm::client;

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("vpsm-client"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    const bool smokeTest = app.arguments().contains(QStringLiteral("--smoke-test"));

    auto platform = createPlatformServices();
    auto tun = platform->createTunDevice();
    auto configurator = platform->createNetworkConfigurator();
    UdpTunnelTransport transport;
    TunnelController controller(*tun, *configurator, transport);
    TunnelViewModel tunnelViewModel(controller);
    HttpControlPlaneClient controlPlane;
    AppViewModel appViewModel(controlPlane, tunnelViewModel);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("viewModel"), QVariant::fromValue(&appViewModel)},
    });
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,&app,
        [&app]() { app.exit(2); },Qt::QueuedConnection
    );
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/VPSM/Client/Main.qml")));
    if (engine.rootObjects().isEmpty()) return 2;

    QObject::connect(&app, &QCoreApplication::aboutToQuit,&controller, &TunnelController::stop);
}