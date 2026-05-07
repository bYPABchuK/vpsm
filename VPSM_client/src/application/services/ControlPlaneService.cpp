#include "ControlPlaneService.hpp"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

ControlPlaneService::ControlPlaneService(QObject *parent)
    : QObject(parent)
{
}

ControlPlaneService::~ControlPlaneService()
{
    delete m_nam;
    m_nam = nullptr;
}

void ControlPlaneService::ensureNetwork()
{
    if (!m_nam) {
        m_nam = new QNetworkAccessManager(this);
    }
}

QString ControlPlaneService::sha256Hex(const QString &text)
{
    const QByteArray digest = QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

QNetworkRequest ControlPlaneService::authedJsonRequest(const QString &path) const
{
    QNetworkRequest req(QUrl(m_baseUrl + path));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QByteArray sessionId = m_sessionId.toUtf8();
    const QByteArray sessionKey = m_sessionKey.toUtf8();

    req.setRawHeader("sessionId", sessionId);
    req.setRawHeader("sessionKey", sessionKey);

    req.setRawHeader("SessionId", sessionId);
    req.setRawHeader("SessionKey", sessionKey);
    req.setRawHeader("X-Session-Id", sessionId);
    req.setRawHeader("X-Session-Key", sessionKey);
    return req;
}

void ControlPlaneService::checkHealth(const QString &baseUrl) { Q_UNUSED(baseUrl) }

void ControlPlaneService::login(const QString &baseUrl, const QString &nick, const QString &password)
{
    ensureNetwork();
    m_peerId = 0;
    m_sessionId.clear();
    m_sessionKey.clear();

    m_baseUrl = baseUrl.trimmed();
    if (!m_baseUrl.startsWith("http://") && !m_baseUrl.startsWith("https://")) m_baseUrl = "http://" + m_baseUrl;
    if (m_baseUrl.endsWith('/')) m_baseUrl.chop(1);

    QNetworkRequest req(QUrl(m_baseUrl + "/user/login"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QJsonObject body{{"nick", nick}, {"password", password}};
    auto *reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) { emit loginFinished(false, reply->errorString()); reply->deleteLater(); return; }
        const auto doc = QJsonDocument::fromJson(payload);
        const auto obj = doc.object();
        if (!obj.value("ok").toBool(false)) { emit loginFinished(false, obj.value("error").toString("login failed")); reply->deleteLater(); return; }

        const QString payloadText = QString::fromUtf8(payload);

        const auto jsonNumberOrText = [&obj, &payloadText](const QString &fieldName) -> QString {
            // 1) Prefer raw payload token (exact integer text, no float conversion risk).
            {
                const QRegularExpression rx(
                    QStringLiteral("\"%1\"\\s*:\\s*\"?(-?\\d+)\"?")
                        .arg(QRegularExpression::escape(fieldName)));
                const auto match = rx.match(payloadText);
                if (match.hasMatch()) {
                    return match.captured(1).trimmed();
                }
            }

            // 2) Fallback to parsed JSON.
            const auto value = obj.value(fieldName);
            if (value.isString()) {
                return value.toString().trimmed();
            }
            if (value.isDouble()) {
                const qint64 i = static_cast<qint64>(value.toDouble());
                if (i != 0) {
                    return QString::number(i);
                }
            }
            return QString();
        };

        const QString peerIdText = jsonNumberOrText(QStringLiteral("peerId"));
        {
            bool okPeerId = false;
            const qint64 parsedPeerId = peerIdText.toLongLong(&okPeerId);
            m_peerId = okPeerId ? parsedPeerId : 0;
        }
        m_sessionId = jsonNumberOrText(QStringLiteral("sessionId"));
        m_sessionKey = jsonNumberOrText(QStringLiteral("sessionKey"));

        if (m_peerId <= 0 || m_sessionId.isEmpty() || m_sessionKey.isEmpty()) {
            emit loginFinished(false, QStringLiteral("login failed: invalid sessionId/sessionKey in response"));
            reply->deleteLater();
            return;
        }

        emit loginFinished(true, QStringLiteral("OK"));
        reply->deleteLater();
    });
}

void ControlPlaneService::createNetwork(const QString &name, const QString &password)
{
    ensureNetwork();
    if (m_baseUrl.isEmpty() || m_sessionId.isEmpty() || m_sessionKey.isEmpty() || m_peerId <= 0) {
        emit networkCreated(false, 0, QStringLiteral("not_authenticated: login required"));
        return;
    }
    QNetworkRequest req = authedJsonRequest("/network/create");
    QJsonObject body{{"ownerPeerId", static_cast<double>(m_peerId)}, {"name", name}, {"passwordHash", sha256Hex(password)}};
    auto *reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        const bool ok = (reply->error() == QNetworkReply::NoError) && obj.value("ok").toBool(false);
        emit networkCreated(ok, static_cast<qint64>(obj.value("networkId").toDouble()), ok ? QStringLiteral("OK") : obj.value("error").toString(reply->errorString()));
        reply->deleteLater();
    });
}

void ControlPlaneService::joinNetwork(qint64 networkId, const QString &password)
{
    ensureNetwork();
    if (m_baseUrl.isEmpty() || m_sessionId.isEmpty() || m_sessionKey.isEmpty() || m_peerId <= 0) {
        emit networkJoined(false, networkId, QStringLiteral("not_authenticated: login required"));
        return;
    }
    QNetworkRequest req = authedJsonRequest(QStringLiteral("/user/networks/%1/members/%2").arg(networkId).arg(m_peerId));
    QJsonObject body{{"passwordHash", sha256Hex(password)}};
    auto *reply = m_nam->sendCustomRequest(req, "PUT", QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, networkId]() {
        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        const bool ok = (reply->error() == QNetworkReply::NoError) && obj.value("ok").toBool(false);
        emit networkJoined(ok, networkId, ok ? QStringLiteral("OK") : obj.value("error").toString(reply->errorString()));
        reply->deleteLater();
    });
}

void ControlPlaneService::listUserNetworks()
{
    ensureNetwork();
    if (m_baseUrl.isEmpty() || m_sessionId.isEmpty() || m_sessionKey.isEmpty() || m_peerId <= 0) {
        emit networksListed(false, {}, {}, QStringLiteral("not_authenticated: login required"));
        return;
    }
    QNetworkRequest req = authedJsonRequest(QStringLiteral("/user/%1/network-list").arg(m_peerId));
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QList<qint64> ids;
        QMap<qint64, QString> names;
        QString msg;
        bool ok = false;
        if (reply->error() == QNetworkReply::NoError) {
            const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray arr = obj.value("networks").toArray();
            if (arr.isEmpty()) arr = obj.value("items").toArray();
            if (arr.isEmpty()) arr = obj.value("data").toArray();
            for (const auto &v : arr) {
                if (v.isDouble()) {
                    ids.push_back(static_cast<qint64>(v.toDouble()));
                    continue;
                }
                if (v.isObject()) {
                    const auto networkObj = v.toObject();
                    qint64 id = static_cast<qint64>(networkObj.value("id").toDouble());
                    if (id <= 0) {
                        id = static_cast<qint64>(networkObj.value("networkId").toDouble());
                    }
                    if (id > 0) {
                        ids.push_back(id);
                        const QString networkName = networkObj.value("name").toString().trimmed();
                        if (!networkName.isEmpty()) {
                            names.insert(id, networkName);
                        }
                    }
                }
            }
            ok = true;
            msg = QStringLiteral("OK");
        } else msg = reply->errorString();
        emit networksListed(ok, ids, names, msg);
        reply->deleteLater();
    });
}

void ControlPlaneService::listUserNetworkPeers()
{
    ensureNetwork();
    if (m_baseUrl.isEmpty() || m_sessionId.isEmpty() || m_sessionKey.isEmpty() || m_peerId <= 0) {
        emit networkPeersListed(false, {}, QStringLiteral("not_authenticated: login required"));
        return;
    }
    QNetworkRequest req = authedJsonRequest(QStringLiteral("/user/%1/network-peers-list").arg(m_peerId));
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QMap<qint64, QList<NetworkPeerInfo>> out;
        QString msg;
        bool ok = false;
        if (reply->error() == QNetworkReply::NoError) {
            const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
            const auto networks = obj.value("networks").toArray();
            for (const auto &n : networks) {
                const auto no = n.toObject();
                const qint64 nid = static_cast<qint64>(no.value("id").toDouble());
                QList<NetworkPeerInfo> peers;
                for (const auto &p : no.value("peers").toArray()) {
                    const auto po = p.toObject();
                    const qint64 peerId = static_cast<qint64>(po.value("peerId").toDouble());
                    const QString nickname = po.value("nickname").toString(po.value("nick").toString());
                    const QString vip = po.value("vip").toVariant().toString();
                    peers.push_back(NetworkPeerInfo{peerId, nickname, vip});
                }
                out.insert(nid, peers);
            }
            ok = true;
            msg = QStringLiteral("OK");
        } else msg = reply->errorString();
        emit networkPeersListed(ok, out, msg);
        reply->deleteLater();
    });
}
