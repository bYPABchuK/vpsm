#include "AppViewModel.hpp"

#include <QVariantMap>

#include <algorithm>

namespace vpsm::client {
    AppViewModel::AppViewModel(IControlPlaneClient& controlPlane, TunnelViewModel& tunnel, QObject* parent) 
    : QObject(parent), controlPlane_(controlPlane), tunnel_(tunnel) {
        connect(&controlPlane_, &IControlPlaneClient::busyChanged,
                this, &AppViewModel::busyChanged);
        connect(&controlPlane_, &IControlPlaneClient::serverProbeCompleted,
                this, [this](const QString& error) {
            serverConnected_ = error.isEmpty();
            setError(error);
            setStatus(error.isEmpty() ? QStringLiteral("Server connected")
                                      : QStringLiteral("Server connection failed"));
            emit serverConnectedChanged();
        });
        connect(&controlPlane_, &IControlPlaneClient::loginCompleted,
                this, [this](const Session& session, const QString& error) {
            if (!error.isEmpty()) {
                session_ = {};
                setError(error);
                setStatus(QStringLiteral("Authentication failed"));
                emit authenticatedChanged();
                return;
            }
            session_ = session;
            setError({});
            setStatus(QStringLiteral("Authenticated"));
            emit authenticatedChanged();
        });
        connect(&controlPlane_, &IControlPlaneClient::networksListed,
                this, [this](const QList<VirtualNetwork>& networks, const QString& error) {
            applyNetworks(networks, error);
        });
        connect(&controlPlane_, &IControlPlaneClient::networkPeersListed,
                this, [this](const QList<VirtualNetwork>& networks, const QString& error) {
            if (!error.isEmpty()) {
                setError(error);
                setStatus(QStringLiteral("Peer list failed"));
                return;
            }
            applyNetworks(networks, {});
        });
        connect(&controlPlane_, &IControlPlaneClient::networkCreated,
                this, [this](quint32, const QString& error) {
            setError(error);
            if (!error.isEmpty()) { setStatus(QStringLiteral("Network creation failed")); return; }
            setStatus(QStringLiteral("Network created"));
            refreshNetworkPeers();
        });
        connect(&controlPlane_, &IControlPlaneClient::networkMutationCompleted,
                this, [this](const QString& error) {
            setError(error);
            if (!error.isEmpty()) { setStatus(QStringLiteral("Network operation failed")); return; }
            setStatus(QStringLiteral("Network updated"));
            refreshNetworkPeers();
        });
        connect(&controlPlane_, &IControlPlaneClient::networkJoined,
                this, [this](const VirtualNetwork& network, const QString& error) {
            if (!error.isEmpty()) {
                setError(error);
                setStatus(QStringLiteral("Network join failed"));
                return;
            }
            if (!startTunnelAfterJoin_) {
                setError({});
                setStatus(QStringLiteral("Network joined"));
                refreshNetworkPeers();
                return;
            }
            startTunnelAfterJoin_ = false;
            if (!tunnel_.connectTunnel(session_, network, pendingRouter_, pendingInterfaceName_)) {
                return;
            }
            setError({});
            setStatus(QStringLiteral("Registering tunnel endpoint"));
        });
        connect(&controlPlane_, &IControlPlaneClient::loggedOut, this, [this]() {
            session_ = {};
            networks_.clear();
            networkItems_.clear();
            setStatus(QStringLiteral("Logged out"));
            emit authenticatedChanged();
            emit networksChanged();
        });
        connect(&tunnel_, &TunnelViewModel::stateChanged, this, [this]() {
            if (tunnel_.connected()) setStatus(QStringLiteral("Connected"));
            else if (tunnel_.state() == static_cast<int>(TunnelState::Error)) {
                setError(tunnel_.errorText());
                setStatus(QStringLiteral("Tunnel error"));
            }
        });
    }

    void AppViewModel::connectServer(const QString& server) {
        if (controlPlane_.busy()) return;
        tunnel_.disconnectTunnel();
        serverConnected_ = false;
        session_ = {};
        networks_.clear();
        networkItems_.clear();
        emit serverConnectedChanged();
        emit authenticatedChanged();
        emit networksChanged();
        setError({});
        setStatus(QStringLiteral("Connecting to server"));
        controlPlane_.probeServer(server);
    }

    void AppViewModel::login(const QString& server, const QString& nickname, const QString& password) {
        if (controlPlane_.busy()) return;
        if (!serverConnected_) {
            setError(QStringLiteral("Connect to the server first"));
            return;
        }
        setError({});
        setStatus(QStringLiteral("Authenticating"));
        controlPlane_.login(server, nickname, password);
    }

    void AppViewModel::connectAvailableNetwork(
        quint32 networkId, const QString& routerAddress, quint16 routerPort,
        quint16 localPort, const QString& interfaceName
    ) {
        if (!session_.isValid()) {
            setError(QStringLiteral("User authentication is required"));
            return;
        }
        const auto selected = std::find_if(networks_.cbegin(), networks_.cend(),
            [networkId](const VirtualNetwork& network) { return network.networkId == networkId; });
        if (selected == networks_.cend()) {
            setError(QStringLiteral("Select an available network"));
            return;
        }
        pendingRouter_ = RouterEndpoint{QHostAddress(routerAddress), routerPort, localPort};
        pendingInterfaceName_ = interfaceName;
        if (!pendingRouter_.isValid()) {
            setError(QStringLiteral("Invalid router endpoint"));
            return;
        }
        setError({});
        if (tunnel_.connectTunnel(session_, *selected, pendingRouter_, pendingInterfaceName_))
            setStatus(QStringLiteral("Registering tunnel endpoint"));
    }

    void AppViewModel::refreshNetworks() {
        if (!session_.isValid() || controlPlane_.busy()) return;
        setStatus(QStringLiteral("Loading networks"));
        controlPlane_.listNetworks();
    }

    void AppViewModel::refreshNetworkPeers() {
        if (!session_.isValid() || controlPlane_.busy()) return;
        setStatus(QStringLiteral("Loading networks and peers"));
        controlPlane_.listNetworkPeers();
    }

    void AppViewModel::createNetwork(const QString& name, const QString& password) {
        if (!session_.isValid() || controlPlane_.busy() || name.trimmed().isEmpty()) return;
        setStatus(QStringLiteral("Creating network"));
        controlPlane_.createNetwork(name, password);
    }

    void AppViewModel::joinNetwork(const QString& name, const QString& password) {
        if (!session_.isValid() || controlPlane_.busy() || name.trimmed().isEmpty()) return;
        setStatus(QStringLiteral("Joining network"));
        startTunnelAfterJoin_ = false;
        controlPlane_.joinNetworkByName(name.trimmed(), password);
    }

    void AppViewModel::leaveNetwork(quint32 networkId) {
        if (!session_.isValid() || controlPlane_.busy() || networkId == 0) return;
        if (tunnel_.connected()) disconnectTunnel();
        setStatus(QStringLiteral("Leaving network"));
        controlPlane_.leaveNetwork(networkId, session_.peerId);
    }

    void AppViewModel::deleteNetwork(quint32 networkId) {
        if (!session_.isValid() || controlPlane_.busy() || networkId == 0) return;
        if (tunnel_.connected()) disconnectTunnel();
        setStatus(QStringLiteral("Deleting network"));
        controlPlane_.deleteNetwork(networkId);
    }

    void AppViewModel::removePeer(quint32 networkId, const QString& peerId) {
        bool ok = false;
        const auto value = peerId.toULongLong(&ok);
        if (!session_.isValid() || controlPlane_.busy() || networkId == 0 || !ok || value == 0) return;
        setStatus(QStringLiteral("Removing network member"));
        controlPlane_.leaveNetwork(networkId, value);
    }

    void AppViewModel::connectNetwork(
        quint32 networkId,
        const QString& networkPassword,
        const QString& routerAddress,
        quint16 routerPort,
        quint16 localPort,
        const QString& interfaceName
    ) {
        if (!session_.isValid() || controlPlane_.busy()) {
            setError(QStringLiteral("Login is required before connecting a network"));
            return;
        }
        pendingRouter_ = RouterEndpoint{QHostAddress(routerAddress), routerPort, localPort};
        pendingInterfaceName_ = interfaceName;
        if (!pendingRouter_.isValid()) {
            setError(QStringLiteral("Invalid router endpoint"));
            return;
        }
        setError({});
        setStatus(QStringLiteral("Joining network"));
        startTunnelAfterJoin_ = true;
        controlPlane_.joinNetwork(networkId, networkPassword);
    }

    void AppViewModel::disconnectTunnel() {
        tunnel_.disconnectTunnel();
        setStatus(QStringLiteral("Disconnected"));
    }

    void AppViewModel::logout() {
        tunnel_.disconnectTunnel();
        if (controlPlane_.busy()) return;
        controlPlane_.logout();
    }

    void AppViewModel::setStatus(const QString& status) {
        if (statusText_ == status) return;
        statusText_ = status;
        emit statusTextChanged();
    }

    void AppViewModel::setError(const QString& error) {
        if (errorText_ == error) return;
        errorText_ = error;
        emit errorTextChanged();
    }

    void AppViewModel::applyNetworks(const QList<VirtualNetwork>& networks, const QString& error) {
        networks_ = error.isEmpty() ? networks : QList<VirtualNetwork>{};
        QSet<quint32> allowedNetworkIds;
        QHash<quint32, quint32> destinationNetworks;
        networkItems_.clear();
        for (const auto& network : networks_) {
            allowedNetworkIds.insert(network.networkId);
            QVariantList peers;
            for (const auto& peer : network.peers) {
                const auto vip = peer.address.toIPv4Address();
                if (vip != network.localAddress.toIPv4Address() && !destinationNetworks.contains(vip))
                    destinationNetworks.insert(vip, network.networkId);
                peers.push_back(QVariantMap{
                    {QStringLiteral("peerId"), QString::number(peer.peerId)},
                    {QStringLiteral("address"), peer.address.toString()},
                    {QStringLiteral("nickname"), peer.nickname},
                });
            }
            networkItems_.push_back(QVariantMap{
                {QStringLiteral("id"), network.networkId},
                {QStringLiteral("name"), network.name},
                {QStringLiteral("ownerPeerId"), QString::number(network.ownerPeerId)},
                {QStringLiteral("isOwner"), network.ownerPeerId == session_.peerId},
                {QStringLiteral("address"), network.localAddress.toString()},
                {QStringLiteral("networkAddress"), network.networkAddress.toString()},
                {QStringLiteral("peers"), peers},
            });
        }
        tunnel_.setAllowedNetworkIds(allowedNetworkIds);
        tunnel_.setDestinationNetworks(destinationNetworks);
        setError(error);
        setStatus(!error.isEmpty() ? QStringLiteral("Network list failed")
            : networks_.isEmpty() ? QStringLiteral("No available networks")
                                  : QStringLiteral("Select a network"));
        emit networksChanged();
    }
}