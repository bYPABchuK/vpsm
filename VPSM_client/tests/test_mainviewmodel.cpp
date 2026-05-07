#include "../src/presentation/viewmodel/MainViewModel.hpp"
#include "../src/application/services/DataPlaneService.hpp"
#include "../src/application/services/ControlPlaneService.hpp"

#include <QSignalSpy>
#include <QtTest/QtTest>

class MainViewModelTest final : public QObject {
    Q_OBJECT
private slots:
    void startStopSessionChangesState();
    void sendPacketWhenSessionStoppedSetsStatus();
    void sendPacketWithInvalidIpv4SetsDataplaneError();
    void sendPacketWithEmptyPayloadProducesTxLine();
};

void MainViewModelTest::startStopSessionChangesState()
{
    DataPlaneService dataPlane;
    ControlPlaneService controlPlane;
    MainViewModel vm(&dataPlane, &controlPlane);

    QSignalSpy connectedSpy(&vm, &MainViewModel::connectedChanged);

    vm.startSession("127.0.0.1", 9000, 9001, 1, 1, "test_mainvm_log.jsonl");
    QVERIFY(vm.connected());
    QVERIFY(connectedSpy.count() >= 1);

    vm.stopSession();
    QVERIFY(!vm.connected());
}

void MainViewModelTest::sendPacketWhenSessionStoppedSetsStatus()
{
    DataPlaneService dataPlane;
    ControlPlaneService controlPlane;
    MainViewModel vm(&dataPlane, &controlPlane);

    vm.sendPacket("10.0.0.1", "10.0.0.2", "10.10.10.0", "payload");
    QCOMPARE(vm.statusText(), QStringLiteral("Cannot send packet: session is not running"));
}

void MainViewModelTest::sendPacketWithInvalidIpv4SetsDataplaneError()
{
    DataPlaneService dataPlane;
    ControlPlaneService controlPlane;
    MainViewModel vm(&dataPlane, &controlPlane);

    QVERIFY(dataPlane.start(1, 1, 19091, "127.0.0.1", 19090));
    vm.sendPacket("bad-ip", "10.0.0.2", "10.10.10.0", "payload");

    QTRY_VERIFY(vm.statusText().contains(QStringLiteral("Dataplane error: Invalid src/dst/network IPv4")));
    dataPlane.stop();
}

void MainViewModelTest::sendPacketWithEmptyPayloadProducesTxLine()
{
    DataPlaneService dataPlane;
    ControlPlaneService controlPlane;
    MainViewModel vm(&dataPlane, &controlPlane);
    QSignalSpy packetLineSpy(&vm, &MainViewModel::packetLineAdded);

    QVERIFY(dataPlane.start(1, 1, 19093, "127.0.0.1", 19092));
    vm.sendPacket("10.0.0.1", "10.0.0.2", "10.10.10.0", "");

    QTRY_VERIFY(packetLineSpy.count() >= 1);
    const auto args = packetLineSpy.takeFirst();
    QVERIFY(args.at(0).toString().contains(QStringLiteral("TX seq=")));
    dataPlane.stop();
}

QTEST_MAIN(MainViewModelTest)
#include "test_mainviewmodel.moc"
