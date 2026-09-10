#include "HttpControlPlaneClient.hpp"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <limits>

namespace vpsm::client {
    HttpControlPlaneClient::HttpControlPlaneClient(QObject* parent)
        : IControlPlaneClient(parent), network_(this) {}

    void HttpControlPlaneClient::probeServer(const QString& server) {
        if (busy_) {
            emit serverProbeCompleted(QStringLiteral("Control Plane request is already in progress"));
            return;
        }
        QUrl url(server.trimmed());
        if (url.scheme().isEmpty()) url = QUrl(QStringLiteral("http://") + server.trimmed());
        if (!url.isValid() || url.host().isEmpty()) {
            emit serverProbeCompleted(QStringLiteral("Invalid server address"));
            return;
        }
        url.setPath({});
        url.setQuery({});
        url.setFragment({});
        baseUrl_ = url;
        session_ = {};

        QNetworkRequest request(baseUrl_.resolved(QUrl(QStringLiteral("/health"))));
        request.setTransferTimeout(10'000);
        setBusy(true);
        auto* reply = network_.get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            QString error;
            if (reply->error() != QNetworkReply::NoError || body.trimmed() != QByteArrayLiteral("ok"))
                error = reply->errorString().isEmpty() ? QStringLiteral("Server health check failed") : reply->errorString();
            reply->deleteLater();
            setBusy(false);
            emit serverProbeCompleted(error);
        });
    }

    void HttpControlPlaneClient::login(
        const QString& server,
        const QString& nickname,
        const QString& password
    ) {
        if (busy_) {
            emit loginCompleted({}, QStringLiteral("Control Plane request is already in progress"));
            return;
        }
        QUrl url(server.trimmed());
        if (url.scheme().isEmpty()) url = QUrl(QStringLiteral("http://") + server.trimmed());
        url.setPath({});
        url.setQuery({});
        url.setFragment({});
        if (!url.isValid() || url.host().isEmpty() || url != baseUrl_) {
            emit loginCompleted({}, QStringLiteral("Connect to the server first"));
            return;
        }
        session_ = {};

        QNetworkRequest request(baseUrl_.resolved(QUrl(QStringLiteral("/user/login"))));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setTransferTimeout(10'000);
        const QJsonObject payload{
            {QStringLiteral("nick"), nickname},
            {QStringLiteral("password"), password},
        };
        setBusy(true);
        auto* reply = network_.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const QByteArray body = reply->readAll();
            const auto document = QJsonDocument::fromJson(body);
            const auto object = document.object();
            const auto peerId = jsonUnsigned(object, QStringLiteral("peerId"));
            const auto sessionId = jsonUnsigned(object, QStringLiteral("sessionId"));
            const auto sessionKey = jsonUnsigned(object, QStringLiteral("sessionKey"));
            const auto dataKey = QByteArray::fromHex(
                object.value(QStringLiteral("dataPlaneKey")).toString().toLatin1()
            );
            QString error;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool()) {
                error = responseError(*reply, body);
            } else if (!peerId || !sessionId || !sessionKey || dataKey.size() != 32) {
                error = QStringLiteral("Invalid login response from router");
            } else {
                session_ = Session{*peerId, *sessionId, *sessionKey, dataKey};
            }
            reply->deleteLater();
            setBusy(false);
            emit loginCompleted(error.isEmpty() ? session_ : Session{}, error);
        });
    }

    void HttpControlPlaneClient::listNetworks() {
        if (busy_) {
            emit networksListed({}, QStringLiteral("Control Plane request is already in progress"));
            return;
        }
        if (!session_.isValid()) {
            emit networksListed({}, QStringLiteral("User authentication is required"));
            return;
        }
        setBusy(true);
        auto* reply = network_.get(authenticatedRequest(
            QStringLiteral("/user/%1/network-list").arg(session_.peerId)
        ));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            QList<VirtualNetwork> networks;
            QString error;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool()) {
                error = responseError(*reply, body);
            } else if (!object.value(QStringLiteral("networks")).isArray()) {
                error = QStringLiteral("Invalid network list response from router");
            } else {
                for (const auto& value : object.value(QStringLiteral("networks")).toArray()) {
                    const auto item = value.toObject();
                    VirtualNetwork network = parseNetwork(item, error);
                    if (!network.isValid()) {
                        if (error.isEmpty()) error = QStringLiteral("Invalid network entry from router");
                        networks.clear();
                        break;
                    }
                    networks.push_back(network);
                }
            }
            reply->deleteLater();
            setBusy(false);
            emit networksListed(networks, error);
        });
    }

    void HttpControlPlaneClient::listNetworkPeers() {
        if (busy_ || !session_.isValid()) {
            emit networkPeersListed({}, QStringLiteral("User authentication is required"));
            return;
        }
        setBusy(true);
        auto* reply = network_.get(authenticatedRequest(
            QStringLiteral("/user/%1/network-peers-list").arg(session_.peerId)));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            QList<VirtualNetwork> networks;
            QString error;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool()) {
                error = responseError(*reply, body);
            } else if (!object.value(QStringLiteral("networks")).isArray()) {
                error = QStringLiteral("Invalid peer list response from router");
            } else {
                for (const auto& value : object.value(QStringLiteral("networks")).toArray()) {
                    auto network = parseNetwork(value.toObject(), error);
                    if (!network.isValid()) break;
                    for (const auto& peerValue : value.toObject().value(QStringLiteral("peers")).toArray()) {
                        const auto peerObject = peerValue.toObject();
                        const auto peerId = jsonUnsigned(peerObject, QStringLiteral("peerId"));
                        const QHostAddress address(peerObject.value(QStringLiteral("vip")).toString());
                        if (!peerId || address.protocol() != QAbstractSocket::IPv4Protocol) {
                            error = QStringLiteral("Invalid peer entry from router");
                            break;
                        }
                        const auto nickname = peerObject.value(QStringLiteral("nickname")).toString(
                            QStringLiteral("peer-%1").arg(*peerId));
                        network.peers.push_back(NetworkPeer{*peerId, address, nickname});
                    }
                    if (!error.isEmpty()) break;
                    networks.push_back(network);
                }
            }
            if (!error.isEmpty()) networks.clear();
            reply->deleteLater();
            setBusy(false);
            emit networkPeersListed(networks, error);
        });
    }

    void HttpControlPlaneClient::createNetwork(const QString& name, const QString& password) {
        if (busy_ || !session_.isValid()) {
            emit networkCreated(0, QStringLiteral("User authentication is required"));
            return;
        }
        const auto passwordHash = QString::fromLatin1(
            QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        const QJsonObject payload{
            {QStringLiteral("name"), name.trimmed()},
            {QStringLiteral("passwordHash"), passwordHash},
        };
        setBusy(true);
        auto* reply = network_.post(authenticatedRequest(QStringLiteral("/network/create")),
                                    QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            const auto id = jsonUnsigned(object, QStringLiteral("networkId"));
            QString error;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool() || !id)
                error = responseError(*reply, body);
            reply->deleteLater();
            setBusy(false);
            emit networkCreated(error.isEmpty() ? static_cast<quint32>(*id) : 0, error);
        });
    }

    void HttpControlPlaneClient::joinNetworkByName(const QString& name, const QString& password) {
        if (busy_ || !session_.isValid() || name.trimmed().isEmpty()) {
            emit networkJoined({}, QStringLiteral("Login and a valid network name are required"));
            return;
        }
        const auto passwordHash = QString::fromLatin1(
            QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
        const QJsonObject payload{
            {QStringLiteral("name"), name.trimmed()},
            {QStringLiteral("passwordHash"), passwordHash},
        };
        setBusy(true);
        auto* reply = network_.sendCustomRequest(authenticatedRequest(QStringLiteral("/network/join")),
            QByteArrayLiteral("PUT"), QJsonDocument(payload).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            QString error;
            VirtualNetwork network;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool()) {
                error = responseError(*reply, body);
            } else {
                const auto id = jsonUnsigned(object, QStringLiteral("networkId"));
                network.networkId = id ? static_cast<quint32>(*id) : 0;
                network.localAddress = QHostAddress(object.value(QStringLiteral("address")).toString());
                network.networkAddress = QHostAddress(object.value(QStringLiteral("networkAddress")).toString());
                network.prefixLength = object.value(QStringLiteral("prefixLength")).toInt();
                network.mtu = object.value(QStringLiteral("mtu")).toInt();
                if (!network.isValid()) error = QStringLiteral("Invalid virtual network configuration from router");
            }
            reply->deleteLater();
            setBusy(false);
            emit networkJoined(error.isEmpty() ? network : VirtualNetwork{}, error);
        });
    }

    void HttpControlPlaneClient::leaveNetwork(quint32 networkId, quint64 peerId) {
        sendMutation(QStringLiteral("/user/networks/%1/members/%2").arg(networkId).arg(peerId));
    }

    void HttpControlPlaneClient::deleteNetwork(quint32 networkId) {
        sendMutation(QStringLiteral("/user/networks/%1").arg(networkId));
    }

    void HttpControlPlaneClient::sendMutation(const QString& path) {
        if (busy_ || !session_.isValid()) {
            emit networkMutationCompleted(QStringLiteral("User authentication is required"));
            return;
        }
        setBusy(true);
        auto* reply = network_.sendCustomRequest(authenticatedRequest(path), QByteArrayLiteral("DELETE"), QByteArrayLiteral("{}"));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            const auto body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            const QString error = reply->error() == QNetworkReply::NoError
                    && object.value(QStringLiteral("ok")).toBool()
                ? QString{} : responseError(*reply, body);
            reply->deleteLater();
            setBusy(false);
            emit networkMutationCompleted(error);
        });
    }

    void HttpControlPlaneClient::joinNetwork(quint32 networkId, const QString& password) {
        if (busy_) {
            emit networkJoined({}, QStringLiteral("Control Plane request is already in progress"));
            return;
        }
        if (!session_.isValid() || networkId == 0) {
            emit networkJoined({}, QStringLiteral("Login and a valid network ID are required"));
            return;
        }
        const QString path = QStringLiteral("/user/networks/%1/members/%2")
            .arg(networkId).arg(session_.peerId);
        auto request = authenticatedRequest(path);
        const auto passwordHash = QString::fromLatin1(
            QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex()
        );
        const QJsonObject payload{{QStringLiteral("passwordHash"), passwordHash}};
        setBusy(true);
        auto* reply = network_.sendCustomRequest(
            request, QByteArrayLiteral("PUT"), QJsonDocument(payload).toJson(QJsonDocument::Compact)
        );
        connect(reply, &QNetworkReply::finished, this, [this, reply, networkId]() {
            const QByteArray body = reply->readAll();
            const auto object = QJsonDocument::fromJson(body).object();
            QString error;
            VirtualNetwork network;
            if (reply->error() != QNetworkReply::NoError || !object.value(QStringLiteral("ok")).toBool()) {
                error = responseError(*reply, body);
            } else {
                const auto returnedId = jsonUnsigned(object, QStringLiteral("networkId"));
                network.networkId = returnedId ? static_cast<quint32>(*returnedId) : networkId;
                network.localAddress = QHostAddress(object.value(QStringLiteral("address")).toString());
                network.networkAddress = QHostAddress(object.value(QStringLiteral("networkAddress")).toString());
                network.prefixLength = object.value(QStringLiteral("prefixLength")).toInt();
                network.mtu = object.value(QStringLiteral("mtu")).toInt();
                if (!network.isValid() || network.networkId != networkId) {
                    error = QStringLiteral("Invalid virtual network configuration from router");
                    network = {};
                }
            }
            reply->deleteLater();
            setBusy(false);
            emit networkJoined(network, error);
        });
    }

    void HttpControlPlaneClient::logout() {
        if (busy_) return;
        if (!session_.isValid()) {
            emit loggedOut();
            return;
        }
        setBusy(true);
        auto* reply = network_.sendCustomRequest(
            authenticatedRequest(QStringLiteral("/user/logout")), QByteArrayLiteral("DELETE"), QByteArrayLiteral("{}")
        );
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            session_ = {};
            setBusy(false);
            emit loggedOut();
        });
    }

    void HttpControlPlaneClient::setBusy(bool busy) {
        if (busy_ == busy) return;
        busy_ = busy;
        emit busyChanged(busy_);
    }

    QNetworkRequest HttpControlPlaneClient::authenticatedRequest(const QString& path) const {
        QNetworkRequest request(baseUrl_.resolved(QUrl(path)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        request.setTransferTimeout(10'000);
        request.setRawHeader("X-Session-Id", QByteArray::number(session_.sessionId));
        request.setRawHeader("X-Session-Key", QByteArray::number(session_.sessionKey));
        return request;
    }

    QString HttpControlPlaneClient::responseError(QNetworkReply& reply, const QByteArray& body) {
        const auto object = QJsonDocument::fromJson(body).object();
        const auto apiError = object.value(QStringLiteral("error")).toString();
        if (!apiError.isEmpty()) return apiError;
        return reply.errorString().isEmpty() ? QStringLiteral("Control Plane request failed") : reply.errorString();
    }

    std::optional<quint64> HttpControlPlaneClient::jsonUnsigned(
        const QJsonObject& object,
        const QString& name
    ) {
        const auto value = object.value(name);
        bool ok = false;
        quint64 result = 0;
        if (value.isString()) result = value.toString().toULongLong(&ok);
        else if (value.isDouble() && value.toDouble() > 0) {
            result = static_cast<quint64>(value.toDouble());
            ok = true;
        }
        return ok && result != 0 ? std::optional<quint64>(result) : std::nullopt;
    }

    VirtualNetwork HttpControlPlaneClient::parseNetwork(const QJsonObject& item, QString& error) {
        VirtualNetwork network;
        const auto id = jsonUnsigned(item, QStringLiteral("id"));
        const auto ownerId = jsonUnsigned(item, QStringLiteral("ownerPeerId"));
        network.networkId = id && *id <= std::numeric_limits<quint32>::max()
            ? static_cast<quint32>(*id) : 0;
        network.name = item.value(QStringLiteral("name")).toString();
        network.ownerPeerId = ownerId.value_or(0);
        network.localAddress = QHostAddress(item.value(QStringLiteral("address")).toString());
        network.networkAddress = QHostAddress(item.value(QStringLiteral("networkAddress")).toString());
        network.prefixLength = item.value(QStringLiteral("prefixLength")).toInt();
        network.mtu = item.value(QStringLiteral("mtu")).toInt();
        if (!network.isValid()) error = QStringLiteral("Invalid network entry from router");
        return network;
    }
}