#pragma once

#include "../../port/IAuthServiceV2.hpp"

#include <cstdint>
#include <unordered_map>

namespace vpsm::server::application {
    class AuthServiceV2 final : public port::IAuthServiceV2 {
    public:
        explicit AuthServiceV2(std::unordered_map<std::uint64_t, std::uint64_t>& highestSeqBySession)
            : highestSeqBySession_(highestSeqBySession) {}

        std::optional<domain::AuthResultV2> verifyAndDecrypt(const domain::PacketIn& packet) override;

    private:
        std::unordered_map<std::uint64_t, std::uint64_t>& highestSeqBySession_;
    };
}
