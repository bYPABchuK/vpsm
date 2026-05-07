#include "mainwindow.h"
#include "PacketRecord.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<PacketRecord>("PacketRecord");
    MainWindow w;
    w.show();
    return QCoreApplication::exec();
}
