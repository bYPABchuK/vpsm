#pragma once

#include "application/ports/IControlPlaneClient.hpp"
#include "presentation/TunnelViewModel.hpp"

#include <QObject>
#include <QVariantList>

namespace vpsm::client {
    class AppViewModel final : public QObject {
        Q_OBJECT
        Q_PROPERTY(bool serverConnected READ serverConnected NOTIFY serverConnectedChanged)
        Q_PROPERTY(bool authenticated READ authenticated NOTIFY authenticatedChanged)
        Q_PROPERTY(QString peerId READ peerId NOTIFY authenticatedChanged)
        Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
        Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
        Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
        Q_PROPERTY(QVariantList networks READ networks NOTIFY networksChanged)
        Q_PROPERTY(TunnelViewModel* tunnel READ tunnel CONSTANT)

    public:
        AppViewModel(
            IControlPlaneClient& controlPlane,
            TunnelViewModel& tunnel,
            QObject* parent = nullptr
        );

        bool serverConnected() const { return serverConnected_; }
        bool authenticated() const { return session_.isValid(); }
        QString peerId() const { return session_.isValid() ? QString::number(session_.peerId) : QString{}; }
        bool busy() const { return controlPlane_.busy(); }
        QString statusText() const { return statusText_; }
        QString errorText() const { return errorText_; }
        QVariantList networks() const { return networkItems_; }
        TunnelViewModel* tunnel() { return &tunnel_; }

        Q_INVOKABLE void connectServer(const QString& server);
        Q_INVOKABLE void login(const QString& server, const QString& nickname, const QString& password);
        Q_INVOKABLE void connectNetwork(
            quint32 networkId,
            const QString& networkPassword,
            const QString& routerAddress,
            quint16 routerPort,
            quint16 localPort,
            const QString& interfaceName = QStringLiteral("vpsm0")
        );
        Q_INVOKABLE void connectAvailableNetwork(
            quint32 networkId,
            const QString& routerAddress,
            quint16 routerPort,
            quint16 localPort,
            const QString& interfaceName = QStringLiteral("vpsm0")
        );
        Q_INVOKABLE void refreshNetworks();
        Q_INVOKABLE void refreshNetworkPeers();
        Q_INVOKABLE void createNetwork(const QString& name, const QString& password);
        Q_INVOKABLE void joinNetwork(const QString& name, const QString& password);
        Q_INVOKABLE void leaveNetwork(quint32 networkId);
        Q_INVOKABLE void deleteNetwork(quint32 networkId);
        Q_INVOKABLE void removePeer(quint32 networkId, const QString& peerId);
        Q_INVOKABLE void disconnectTunnel();
        Q_INVOKABLE void logout();

    signals:
        void serverConnectedChanged();
        void authenticatedChanged();
        void busyChanged();
        void statusTextChanged();
        void errorTextChanged();
        void networksChanged();

    private:
        void setStatus(const QString& status);
        void setError(const QString& error);
        void applyNetworks(const QList<VirtualNetwork>& networks, const QString& error);

        IControlPlaneClient& controlPlane_;
        TunnelViewModel& tunnel_;
        Session session_;
        QList<VirtualNetwork> networks_;
        QVariantList networkItems_;
        RouterEndpoint pendingRouter_;
        QString pendingInterfaceName_;
        QString statusText_ = QStringLiteral("Disconnected");
        QString errorText_;
        bool serverConnected_ = false;
        bool startTunnelAfterJoin_ = false;
    };
}