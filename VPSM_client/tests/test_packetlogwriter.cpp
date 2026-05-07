#include "../src/application/logging/PacketLogWriter.hpp"

#include <QtTest/QtTest>

#include <fstream>

class PacketLogWriterTest final : public QObject {
    Q_OBJECT
private slots:
    void writesJsonLine();
};

void PacketLogWriterTest::writesJsonLine()
{
    const std::string path = "test_packetlogwriter.jsonl";

    PacketLogWriter writer;
    QVERIFY(writer.start(path));

    PacketRecord rec;
    rec.srcVip = "10.0.0.1";
    rec.destVip = "10.0.0.2";
    rec.networkIp = "10.10.10.0";
    rec.seq = 1;
    rec.timestampIso = "2026-01-01T00:00:00Z";
    writer.enqueue(rec);
    writer.stop();

    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    QVERIFY(!line.empty());
    QVERIFY(line.find("srcVip") != std::string::npos);
}

QTEST_MAIN(PacketLogWriterTest)
#include "test_packetlogwriter.moc"
