#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>

struct NetworkPeerInfo {
    qint64 peerId {0};
    QString nickname;
    QString vip;
};

struct LoginResult {
    bool ok {false};
    QString error;
    qint64 peerId {0};
    qint64 sessionId {0};
    qint64 sessionKey {0};
};

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;

class ControlPlaneService final : public QObject {
    Q_OBJECT
public:
    explicit ControlPlaneService(QObject *parent = nullptr);
    ~ControlPlaneService() override;

public slots:
    void checkHealth(const QString &baseUrl);
    void login(const QString &baseUrl, const QString &nick, const QString &password);
    void createNetwork(const QString &name, const QString &password);
    void joinNetwork(qint64 networkId, const QString &password);
    void listUserNetworks();
    void listUserNetworkPeers();

signals:
    void healthChecked(bool ok, const QString &message);
    void loginFinished(bool ok, const QString &message);
    void networksListed(bool ok, const QList<qint64> &networkIds, const QMap<qint64, QString> &networkNames, const QString &message);
    void networkPeersListed(bool ok, const QMap<qint64, QList<NetworkPeerInfo>> &peersByNetwork, const QString &message);
    void networkCreated(bool ok, qint64 networkId, const QString &message);
    void networkJoined(bool ok, qint64 networkId, const QString &message);

private:
    QNetworkAccessManager *m_nam {nullptr};
    QString m_baseUrl;
    qint64 m_peerId {0};
    QString m_sessionId;
    QString m_sessionKey;
    static QString sha256Hex(const QString &text);
    QNetworkRequest authedJsonRequest(const QString &path) const;
    void ensureNetwork();
};
