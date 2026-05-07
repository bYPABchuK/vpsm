#pragma once

#include <QString>
#include <QtGlobal>
#include <QMetaType>

struct PacketRecord {
    QString srcVip;
    QString destVip;
    QString networkIp;
    QString payload;
    quint64 seq {0};
    QString timestampIso;
};

Q_DECLARE_METATYPE(PacketRecord)
