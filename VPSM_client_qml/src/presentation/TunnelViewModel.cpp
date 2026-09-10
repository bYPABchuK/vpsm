#include "TunnelViewModel.hpp"

namespace vpsm::client {
    TunnelViewModel::TunnelViewModel(TunnelController& controller, QObject* parent)
        : QObject(parent), controller_(controller) {
        connect(&controller_, &TunnelController::stateChanged,
                this, &TunnelViewModel::stateChanged);
        connect(&controller_, &TunnelController::errorChanged,
                this, &TunnelViewModel::errorTextChanged);
        connect(&controller_, &TunnelController::activeNetworkChanged,
                this, &TunnelViewModel::networkChanged);
        connect(&controller_, &TunnelController::statisticsChanged,
                this, &TunnelViewModel::statisticsChanged);
    }

    int TunnelViewModel::state() const { return static_cast<int>(controller_.state()); }

    QString TunnelViewModel::stateText() const {
        switch (controller_.state()) {
            case TunnelState::Disconnected: return QStringLiteral("Disconnected");
            case TunnelState::PreparingInterface: return QStringLiteral("Preparing interface");
            case TunnelState::BindingTransport: return QStringLiteral("Binding transport");
            case TunnelState::RegisteringEndpoint: return QStringLiteral("Registering endpoint");
            case TunnelState::Connected: return QStringLiteral("Connected");
            case TunnelState::Disconnecting: return QStringLiteral("Disconnecting");
            case TunnelState::Error: return QStringLiteral("Error");
        }
        return QStringLiteral("Unknown");
    }

    bool TunnelViewModel::connected() const { return controller_.state() == TunnelState::Connected; }
    bool TunnelViewModel::busy() const {
        const auto current = controller_.state();
        return current != TunnelState::Disconnected && current != TunnelState::Connected && current != TunnelState::Error;
    }
    QString TunnelViewModel::errorText() const { return controller_.errorText(); }
    QString TunnelViewModel::interfaceName() const { return controller_.interfaceName(); }
    QString TunnelViewModel::localAddress() const { return controller_.activeNetwork().localAddress.toString(); }
    qulonglong TunnelViewModel::txPackets() const { return controller_.statistics().txPackets; }
    qulonglong TunnelViewModel::rxPackets() const { return controller_.statistics().rxPackets; }
    qulonglong TunnelViewModel::txBytes() const { return controller_.statistics().txBytes; }
    qulonglong TunnelViewModel::rxBytes() const { return controller_.statistics().rxBytes; }
    qulonglong TunnelViewModel::droppedPackets() const { return controller_.statistics().droppedPackets; }

    bool TunnelViewModel::connectTunnel(
        const Session& session,
        const VirtualNetwork& network,
        const RouterEndpoint& router,
        const QString& interfaceName
    ) {
        return controller_.start(session, network, router, interfaceName);
    }

    void TunnelViewModel::disconnectTunnel() {
        controller_.stop();
    }

    void TunnelViewModel::setAllowedNetworkIds(const QSet<quint32>& networkIds) {
        controller_.setAllowedNetworkIds(networkIds);
    }

    void TunnelViewModel::setDestinationNetworks(const QHash<quint32, quint32>& networkByDestinationVip) {
        controller_.setDestinationNetworks(networkByDestinationVip);
    }
}