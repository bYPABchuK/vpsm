#include "ReplayWindow.hpp"

namespace vpsm::client {
    bool ReplayWindow::accept(quint64 sequence) {
        constexpr quint64 windowBits = 64;
        if (sequence == 0) return false;
        if (highest_ == 0) {
            highest_ = sequence;
            bitmap_ = 1;
            return true;
        }
        if (sequence > highest_) {
            const auto shift = sequence - highest_;
            bitmap_ = shift >= windowBits ? 1 : (bitmap_ << shift) | 1;
            highest_ = sequence;
            return true;
        }
        const auto age = highest_ - sequence;
        if (age >= windowBits) return false;
        const quint64 mask = quint64{1} << age;
        if ((bitmap_ & mask) != 0) return false;
        bitmap_ |= mask;
        return true;
    }

    void ReplayWindow::reset() {
        highest_ = 0;
        bitmap_ = 0;
    }
}