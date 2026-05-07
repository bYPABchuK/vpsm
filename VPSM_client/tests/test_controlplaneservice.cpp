#include "../src/application/services/ControlPlaneService.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

Q_DECLARE_METATYPE(NetworkPeerInfo)
Q_DECLARE_METATYPE(QList<NetworkPeerInfo>)
using PeersByNetworkMap = QMap<qint64, QList<NetworkPeerInfo>>;
Q_DECLARE_METATYPE(PeersByNetworkMap)

class ControlPlaneServiceTest final : public QObject {
    Q_OBJECT
private slots:
    void createNetworkRequiresLogin();
    void joinNetworkRequiresLogin();
    void listUserNetworksRequiresLogin();
    void listUserNetworkPeersRequiresLogin();
};

void ControlPlaneServiceTest::createNetworkRequiresLogin()
{
    ControlPlaneService service;
    QSignalSpy createdSpy(&service, &ControlPlaneService::networkCreated);

    service.createNetwork("net-a", "pwd");

    QCOMPARE(createdSpy.count(), 1);
    const auto args = createdSpy.takeFirst();
    QVERIFY(!args.at(0).toBool());
    QCOMPARE(args.at(1).toLongLong(), 0);
    QVERIFY(args.at(2).toString().contains("not_authenticated"));
}

void ControlPlaneServiceTest::joinNetworkRequiresLogin()
{
    ControlPlaneService service;
    QSignalSpy joinedSpy(&service, &ControlPlaneService::networkJoined);

    service.joinNetwork(42, "pwd");

    QCOMPARE(joinedSpy.count(), 1);
    const auto args = joinedSpy.takeFirst();
    QVERIFY(!args.at(0).toBool());
    QCOMPARE(args.at(1).toLongLong(), 42);
    QVERIFY(args.at(2).toString().contains("not_authenticated"));
}

void ControlPlaneServiceTest::listUserNetworksRequiresLogin()
{
    ControlPlaneService service;
    QSignalSpy listedSpy(&service, &ControlPlaneService::networksListed);

    service.listUserNetworks();

    QCOMPARE(listedSpy.count(), 1);
    const auto args = listedSpy.takeFirst();
    QVERIFY(!args.at(0).toBool());
    const auto ids = qvariant_cast<QList<qint64>>(args.at(1));
    QVERIFY(ids.isEmpty());
    const auto names = qvariant_cast<QMap<qint64, QString>>(args.at(2));
    QVERIFY(names.isEmpty());
    QVERIFY(args.at(3).toString().contains("not_authenticated"));
}

void ControlPlaneServiceTest::listUserNetworkPeersRequiresLogin()
{
    qRegisterMetaType<PeersByNetworkMap>();

    ControlPlaneService service;
    QSignalSpy peersSpy(&service, &ControlPlaneService::networkPeersListed);

    service.listUserNetworkPeers();

    QCOMPARE(peersSpy.count(), 1);
    const auto args = peersSpy.takeFirst();
    QVERIFY(!args.at(0).toBool());
    const auto peersByNetwork = qvariant_cast<PeersByNetworkMap>(args.at(1));
    QVERIFY(peersByNetwork.isEmpty());
    QVERIFY(args.at(2).toString().contains("not_authenticated"));
}

QTEST_MAIN(ControlPlaneServiceTest)
#include "test_controlplaneservice.moc"
