#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <unordered_set>
#include <vector>

namespace vpsm::server::application {
    class VipAllocator final {
    public:
        VipAllocator(
            std::uint32_t minVip = 1,
            std::uint32_t maxVip = std::numeric_limits<std::uint32_t>::max()
        ) : minVip_(minVip), maxVip_(maxVip), nextVip_(minVip) {}

        std::optional<std::uint32_t> allocate() {
            while (!freeMinHeap_.empty() &&
                   freeSet_.find(freeMinHeap_.top()) == freeSet_.end()) {
                freeMinHeap_.pop();
            }

            if (!freeMinHeap_.empty()) {
                const auto vip = freeMinHeap_.top();
                freeMinHeap_.pop();
                freeSet_.erase(vip);
                allocated_.insert(vip);
                return vip;
            }

            if (nextVip_ > maxVip_) {
                return std::nullopt;
            }

            const auto vip = nextVip_;
            ++nextVip_;
            allocated_.insert(vip);
            return vip;
        }

        bool reserve(std::uint32_t vip) {
            if (vip < minVip_ || vip > maxVip_) {
                return false;
            }

            if (allocated_.find(vip) != allocated_.end()) {
                return true;
            }

            if (vip >= nextVip_) {
                nextVip_ = vip + 1;
            } else {
                freeSet_.erase(vip);
            }

            allocated_.insert(vip);
            return true;
        }

        bool release(std::uint32_t vip) {
            if (allocated_.erase(vip) == 0) {
                return false;
            }

            if (freeSet_.insert(vip).second) {
                freeMinHeap_.push(vip);
            }

            return true;
        }

    private:
        std::uint32_t minVip_;
        std::uint32_t maxVip_;
        std::uint32_t nextVip_;

        std::priority_queue<
            std::uint32_t,
            std::vector<std::uint32_t>,
            std::greater<std::uint32_t>
        > freeMinHeap_;

        std::unordered_set<std::uint32_t> freeSet_;
        std::unordered_set<std::uint32_t> allocated_;
    };
}