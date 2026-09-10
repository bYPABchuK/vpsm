#pragma once

#include "application/ports/IControlPlaneClient.hpp"

#include <QNetworkAccessManager>
#include <QUrl>

namespace vpsm::client {
    class HttpControlPlaneClient final : public IControlPlaneClient {
        Q_OBJECT
    public:
        explicit HttpControlPlaneClient(QObject* parent = nullptr);

        void probeServer(const QString& server) override;
        void login(const QString& server, const QString& nickname, const QString& password) override;
        void joinNetwork(quint32 networkId, const QString& password) override;
        void joinNetworkByName(const QString& name, const QString& password) override;
        void createNetwork(const QString& name, const QString& password) override;
        void leaveNetwork(quint32 networkId, quint64 peerId) override;
        void deleteNetwork(quint32 networkId) override;
        void listNetworks() override;
        void listNetworkPeers() override;
        void logout() override;
        bool busy() const override { return busy_; }

    private:
        void setBusy(bool busy);
        QNetworkRequest authenticatedRequest(const QString& path) const;
        static QString responseError(QNetworkReply& reply, const QByteArray& body);
        static std::optional<quint64> jsonUnsigned(const QJsonObject& object, const QString& name);
        static VirtualNetwork parseNetwork(const QJsonObject& object, QString& error);
        void sendMutation(const QString& path);

        QNetworkAccessManager network_;
        QUrl baseUrl_;
        Session session_;
        bool busy_ = false;
    };
}