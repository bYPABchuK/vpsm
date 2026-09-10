#include "infrastructure/HttpControlPlaneClient.hpp"

#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest/QtTest>

using namespace vpsm::client;

class HttpControlPlaneTest final : public QObject {
    Q_OBJECT
private slots:
    void serverProbeUserLoginAndListNetworks();
    void authenticatedNetworkManagementUsesRestApi();
};

namespace {
class HttpFixture final : public QObject {
public:
    explicit HttpFixture(QObject* parent = nullptr) : QObject(parent) {
        QObject::connect(&server, &QTcpServer::newConnection, this, [this]() {
            auto* socket = server.nextPendingConnection();
            QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                request += socket->readAll();
                if (!request.contains("\r\n\r\n")) return;
                const auto response = responses.takeFirst();
                const QByteArray header = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
                    QByteArray::number(response.size()) + "\r\nConnection: close\r\n\r\n";
                socket->write(header + response);
                socket->disconnectFromHost();
            });
        });
    }
    bool start() { return server.listen(QHostAddress::LocalHost, 0); }
    QString url() const { return QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()); }
    QTcpServer server;
    QList<QByteArray> responses;
    QByteArray request;
};
}

void HttpControlPlaneTest::serverProbeUserLoginAndListNetworks() {
    HttpFixture fixture;
    QVERIFY(fixture.start());
    fixture.responses << QByteArrayLiteral("ok")
        << QByteArrayLiteral(
        R"({"ok":true,"peerId":"1","sessionId":"9223372036854775808","sessionKey":"9223372036854775809","dataPlaneKey":"4242424242424242424242424242424242424242424242424242424242424242"})")
        << QByteArrayLiteral(
        R"({"ok":true,"networks":[{"id":"1","name":"private","address":"10.240.1.1","networkAddress":"10.240.1.0","prefixLength":24,"mtu":1400}]})");

    HttpControlPlaneClient client;
    QSignalSpy serverSpy(&client, &HttpControlPlaneClient::serverProbeCompleted);
    client.probeServer(fixture.url());
    QTRY_COMPARE_WITH_TIMEOUT(serverSpy.count(), 1, 1000);
    QVERIFY(serverSpy.takeFirst().at(0).toString().isEmpty());
    QVERIFY(fixture.request.startsWith("GET /health"));

    fixture.request.clear();
    QSignalSpy loginSpy(&client, &HttpControlPlaneClient::loginCompleted);
    client.login(fixture.url(), QStringLiteral("alice"), QStringLiteral("pw"));
    QTRY_COMPARE_WITH_TIMEOUT(loginSpy.count(), 1, 1000);
    const auto loginArguments = loginSpy.takeFirst();
    const auto session = qvariant_cast<Session>(loginArguments.at(0));
    QVERIFY(loginArguments.at(1).toString().isEmpty());
    QCOMPARE(session.sessionId, quint64(0x8000000000000000ull));
    QCOMPARE(session.sessionKey, quint64(0x8000000000000001ull));
    QCOMPARE(session.dataPlaneKey.size(), 32);

    fixture.request.clear();
    QSignalSpy listSpy(&client, &HttpControlPlaneClient::networksListed);
    client.listNetworks();
    QTRY_COMPARE_WITH_TIMEOUT(listSpy.count(), 1, 1000);
    const auto listArguments = listSpy.takeFirst();
    const auto networks = qvariant_cast<QList<VirtualNetwork>>(listArguments.at(0));
    QVERIFY(listArguments.at(1).toString().isEmpty());
    QCOMPARE(networks.size(), 1);
    const auto network = networks.front();
    QVERIFY(network.isValid());
    QCOMPARE(network.localAddress.toString(), QStringLiteral("10.240.1.1"));
    QCOMPARE(network.networkAddress.toString(), QStringLiteral("10.240.1.0"));
    QCOMPARE(network.prefixLength, 24);
    QCOMPARE(network.mtu, 1400);
    const auto lowerRequest = fixture.request.toLower();
    QVERIFY(lowerRequest.contains("x-session-id: 9223372036854775808"));
    QVERIFY(lowerRequest.contains("x-session-key: 9223372036854775809"));
    QVERIFY(fixture.request.startsWith("GET /user/1/network-list"));
}

void HttpControlPlaneTest::authenticatedNetworkManagementUsesRestApi() {
    HttpFixture fixture;
    QVERIFY(fixture.start());
    const auto login = QByteArrayLiteral(
        R"({"ok":true,"peerId":"1","sessionId":"2","sessionKey":"3","dataPlaneKey":"4242424242424242424242424242424242424242424242424242424242424242"})");
    fixture.responses << QByteArrayLiteral("ok") << login
        << QByteArrayLiteral(R"({"ok":true,"networkId":"7"})")
        << QByteArrayLiteral(R"({"ok":true,"networkId":"7","address":"10.240.0.1","networkAddress":"10.240.0.0","prefixLength":16,"mtu":1400})")
        << QByteArrayLiteral(R"({"ok":true,"networks":[{"id":"7","name":"team","ownerPeerId":"1","address":"10.240.0.1","networkAddress":"10.240.0.0","prefixLength":16,"mtu":1400,"peers":[{"peerId":"1","vip":"10.240.0.1","nickname":"alice"},{"peerId":"9","vip":"10.240.0.2","nickname":"bob"}]}]})")
        << QByteArrayLiteral(R"({"ok":true})") << QByteArrayLiteral(R"({"ok":true})");
    HttpControlPlaneClient client;
    QSignalSpy probeSpy(&client, &HttpControlPlaneClient::serverProbeCompleted);
    client.probeServer(fixture.url());
    QTRY_COMPARE(probeSpy.count(), 1);
    fixture.request.clear();
    QSignalSpy loginSpy(&client, &HttpControlPlaneClient::loginCompleted);
    client.login(fixture.url(), QStringLiteral("alice"), QStringLiteral("pw"));
    QTRY_COMPARE(loginSpy.count(), 1);

    fixture.request.clear();
    QSignalSpy createSpy(&client, &HttpControlPlaneClient::networkCreated);
    client.createNetwork(QStringLiteral("team"), QStringLiteral("secret"));
    QTRY_COMPARE(createSpy.count(), 1);
    QVERIFY(fixture.request.startsWith("POST /network/create"));
    QVERIFY(fixture.request.contains("\"name\":\"team\""));
    QVERIFY(!fixture.request.contains("ownerPeerId"));
    QVERIFY(!fixture.request.contains("\"passwordHash\":\"secret\""));

    fixture.request.clear();
    QSignalSpy joinSpy(&client, &HttpControlPlaneClient::networkJoined);
    client.joinNetworkByName(QStringLiteral("team"), QStringLiteral("secret"));
    QTRY_COMPARE(joinSpy.count(), 1);
    QVERIFY(fixture.request.startsWith("PUT /network/join"));
    QVERIFY(fixture.request.contains("\"name\":\"team\""));

    fixture.request.clear();
    QSignalSpy peersSpy(&client, &HttpControlPlaneClient::networkPeersListed);
    client.listNetworkPeers();
    QTRY_COMPARE(peersSpy.count(), 1);
    const auto networks = qvariant_cast<QList<VirtualNetwork>>(peersSpy.takeFirst().at(0));
    QCOMPARE(networks.size(), 1);
    QCOMPARE(networks.front().ownerPeerId, 1u);
    QCOMPARE(networks.front().peers.size(), 2);
    QCOMPARE(networks.front().peers.front().nickname, QStringLiteral("alice"));
    QVERIFY(fixture.request.startsWith("GET /user/1/network-peers-list"));

    fixture.request.clear();
    QSignalSpy mutationSpy(&client, &HttpControlPlaneClient::networkMutationCompleted);
    client.leaveNetwork(7, 9);
    QTRY_COMPARE(mutationSpy.count(), 1);
    QVERIFY(fixture.request.startsWith("DELETE /user/networks/7/members/9"));
    fixture.request.clear();
    client.deleteNetwork(7);
    QTRY_COMPARE(mutationSpy.count(), 2);
    QVERIFY(fixture.request.startsWith("DELETE /user/networks/7 "));
}

QTEST_MAIN(HttpControlPlaneTest)
#include "test_httpcontrolplane.moc"