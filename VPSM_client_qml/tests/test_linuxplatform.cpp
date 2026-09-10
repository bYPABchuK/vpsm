#include "platform/linux/LinuxTunDevice.hpp"
#include "platform/linux/PrivilegedHelperProtocol.hpp"
#include "infrastructure/UdpTunnelTransport.hpp"

#include <QNetworkDatagram>
#include <QSignalSpy>
#include <QUdpSocket>
#include <QtTest/QtTest>

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace vpsm::client;

class LinuxPlatformTest final : public QObject {
    Q_OBJECT
private slots:
    void invalidInterfaceNameFailsWithoutSideEffects();
    void writeBeforeCreateFails();
    void udpTransportUsesOneBoundSocketAndFiltersSender();
    void helperProtocolTransfersFileDescriptor();
};

void LinuxPlatformTest::invalidInterfaceNameFailsWithoutSideEffects() {
    LinuxTunDevice tun;
    QString error;
    QVERIFY(!tun.create(QString(100, 'x'), error));
    QVERIFY(!error.isEmpty());
    QVERIFY(tun.interfaceName().isEmpty());
}

void LinuxPlatformTest::writeBeforeCreateFails() {
    LinuxTunDevice tun;
    QString error;
    QVERIFY(!tun.writePacket(QByteArray(20, '\0'), error));
    QVERIFY(error.contains(QStringLiteral("not open")));
}

void LinuxPlatformTest::udpTransportUsesOneBoundSocketAndFiltersSender() {
    QUdpSocket router;
    QVERIFY(router.bind(QHostAddress::LocalHost, 0));
    QUdpSocket portProbe;
    QVERIFY(portProbe.bind(QHostAddress::LocalHost, 0));
    const auto clientPort = portProbe.localPort();
    portProbe.close();

    UdpTunnelTransport transport;
    QSignalSpy receivedSpy(&transport, &UdpTunnelTransport::datagramReceived);
    QString error;
    QVERIFY(transport.open(RouterEndpoint{
        QHostAddress::LocalHost, router.localPort(), clientPort
    }, error));
    QVERIFY(transport.sendDatagram(QByteArray("request"), error));
    QTRY_VERIFY_WITH_TIMEOUT(router.hasPendingDatagrams(), 500);
    const auto request = router.receiveDatagram();
    QCOMPARE(request.data(), QByteArray("request"));
    QCOMPARE(request.senderPort(), clientPort);

    QCOMPARE(router.writeDatagram(QByteArray("response"), QHostAddress::LocalHost, clientPort), qint64(8));
    QTRY_COMPARE_WITH_TIMEOUT(receivedSpy.count(), 1, 500);
    QCOMPARE(receivedSpy.takeFirst().at(0).toByteArray(), QByteArray("response"));

    QUdpSocket attacker;
    QVERIFY(attacker.bind(QHostAddress::LocalHost, 0));
    QCOMPARE(attacker.writeDatagram(QByteArray("forged"), QHostAddress::LocalHost, clientPort), qint64(6));
    QTest::qWait(30);
    QCOMPARE(receivedSpy.count(), 0);
    transport.close();
}

void LinuxPlatformTest::helperProtocolTransfersFileDescriptor() {
    int sockets[2] = {-1, -1};
    QVERIFY(::socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets) == 0);
    const int source = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    QVERIFY(source >= 0);
    vpsm::client::helper_protocol::Response sent;
    QVERIFY(vpsm::client::helper_protocol::sendResponse(sockets[0], sent, source));
    vpsm::client::helper_protocol::Response received;
    int transferred = -1;
    QVERIFY(vpsm::client::helper_protocol::receiveResponse(sockets[1], received, transferred));
    QVERIFY(transferred >= 0);
    QCOMPARE(::fcntl(transferred, F_GETFD) >= 0, true);
    ::close(transferred);
    ::close(source);
    ::close(sockets[0]);
    ::close(sockets[1]);
}

QTEST_MAIN(LinuxPlatformTest)
#include "test_linuxplatform.moc"