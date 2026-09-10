#pragma once

#include <QtGlobal>

namespace vpsm::client {
    class ReplayWindow {
    public:
        bool accept(quint64 sequence);
        void reset();

    private:
        quint64 highest_ = 0;
        quint64 bitmap_ = 0;
    };
}