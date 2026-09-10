#pragma once

#include "domain/TunnelTypes.hpp"

#include <QList>
#include <QObject>

namespace vpsm::client {
    class IControlPlaneClient : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;
        ~IControlPlaneClient() override = default;

        virtual void probeServer(const QString& server) = 0;
        virtual void login(const QString& server, const QString& nickname, const QString& password) = 0;
        virtual void joinNetwork(quint32 networkId, const QString& password) = 0;
        virtual void joinNetworkByName(const QString& name, const QString& password) = 0;
        virtual void createNetwork(const QString& name, const QString& password) = 0;
        virtual void leaveNetwork(quint32 networkId, quint64 peerId) = 0;
        virtual void deleteNetwork(quint32 networkId) = 0;
        virtual void listNetworks() = 0;
        virtual void listNetworkPeers() = 0;
        virtual void logout() = 0;

        virtual bool busy() const = 0;

    signals:
        void busyChanged(bool busy);
        void serverProbeCompleted(const QString& error);
        void loginCompleted(const Session& session, const QString& error);
        void networkJoined(const VirtualNetwork& network, const QString& error);
        void networkCreated(quint32 networkId, const QString& error);
        void networkMutationCompleted(const QString& error);
        void networksListed(const QList<VirtualNetwork>& networks, const QString& error);
        void networkPeersListed(const QList<VirtualNetwork>& networks, const QString& error);
        void loggedOut();
    };
}