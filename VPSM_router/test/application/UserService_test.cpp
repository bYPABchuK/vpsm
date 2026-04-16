#include "../../src/application/DataPlane/UserService.hpp"
#include "../../src/port/UserServiceResult.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <unordered_map>

namespace {
    using vpsm::server::application::UserService;
    using vpsm::server::domain::VNetwork;

    class PeerRepositoryFake final : public vpsm::server::port::IPeerRepository {
    public:
        std::optional<std::uint64_t> createPeer(const std::string&, const std::string&) override {
            return createPeerResult;
        }

        bool deletePeer(std::uint64_t peerId) override {
            deletedPeerId = peerId;
            return deletePeerResult;
        }

        std::optional<std::string> getPasswordHash(std::uint64_t) const override {
            return std::nullopt;
        }

        std::optional<std::uint64_t> findPeerIdByNickname(const std::string&) const override {
            return std::nullopt;
        }

        bool exists(std::uint64_t peerId) const override {
            const auto it = existsByPeerId.find(peerId);
            return it != existsByPeerId.end() ? it->second : false;
        }

        std::optional<std::uint64_t> createPeerResult = 1;
        bool deletePeerResult = true;
        mutable std::unordered_map<std::uint64_t, bool> existsByPeerId;
        std::uint64_t deletedPeerId = 0;
    };

    class NetworkRepositoryFake final : public vpsm::server::port::IVNetworkRepository {
    public:
        std::optional<std::uint64_t> createNetwork(
            std::uint64_t,
            const std::string&,
            const std::string&
        ) override {
            return createNetworkResult;
        }

        bool deleteNetwork(std::uint64_t networkId) override {
            deletedNetworkId = networkId;
            return deleteNetworkResult;
        }

        std::optional<VNetwork> getNetwork(std::uint64_t networkId) const override {
            const auto it = networksById.find(networkId);
            if (it == networksById.end()) {
                return std::nullopt;
            }

            return it->second;
        }

        std::vector<VNetwork> listNetworks() const override {
            std::vector<VNetwork> out;
            out.reserve(networksById.size());
            for (const auto& [_, n] : networksById) {
                out.push_back(n);
            }
            return out;
        }

        bool exists(std::uint64_t networkId) const override {
            const auto it = existsByNetworkId.find(networkId);
            return it != existsByNetworkId.end() ? it->second : false;
        }

        std::optional<std::uint64_t> createNetworkResult = 10;
        bool deleteNetworkResult = true;
        mutable std::unordered_map<std::uint64_t, VNetwork> networksById;
        mutable std::unordered_map<std::uint64_t, bool> existsByNetworkId;
        std::uint64_t deletedNetworkId = 0;
    };

    class MembershipStoreFake final : public vpsm::server::port::IMembershipStore {
    public:
        std::optional<std::uint32_t> allocateVip(std::uint32_t networkId, std::uint64_t peerId) override {
            allocateVipCalls++;
            lastNetworkId = networkId;
            lastPeerId = peerId;
            return allocateVipResult;
        }

        bool releaseVip(std::uint32_t networkId, std::uint64_t peerId) override {
            releaseVipCalls++;
            lastNetworkId = networkId;
            lastPeerId = peerId;
            return releaseVipResult;
        }

        bool bindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }
        bool unbindPeer(std::uint32_t, std::uint64_t, std::uint32_t) override { return false; }

        bool hasPeer(std::uint32_t networkId, std::uint64_t peerId) const override {
            const auto key = (static_cast<std::uint64_t>(networkId) << 32) | peerId;
            const auto it = hasPeerByKey.find(key);
            return it != hasPeerByKey.end() ? it->second : false;
        }

        std::optional<std::uint32_t> resolveVip(std::uint32_t networkId, std::uint64_t peerId) const override {
            const auto key = (static_cast<std::uint64_t>(networkId) << 32) | peerId;
            const auto it = resolveVipByKey.find(key);
            if (it == resolveVipByKey.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        std::optional<std::uint64_t> resolvePeer(std::uint32_t, std::uint32_t) const override {
            return std::nullopt;
        }

        std::vector<vpsm::server::domain::Peer> listPeers(std::uint32_t) const override {
            return {};
        }

        std::optional<std::uint32_t> allocateVipResult = 123;
        bool releaseVipResult = true;
        mutable std::unordered_map<std::uint64_t, bool> hasPeerByKey;
        mutable std::unordered_map<std::uint64_t, std::uint32_t> resolveVipByKey;

        int allocateVipCalls = 0;
        int releaseVipCalls = 0;
        std::uint32_t lastNetworkId = 0;
        std::uint64_t lastPeerId = 0;
    };

    VNetwork makeNetwork(std::uint64_t id, std::uint64_t ownerPeerId, const std::string& passwordHash) {
        VNetwork net;
        net.id = id;
        net.name = "net";
        net.password_hash = passwordHash;
        net.owner.peerId = ownerPeerId;
        net.owner.vip = 1;
        return net;
    }

    TEST(UserServiceTest, createNetwork_ownerNeSushestvuet_PeerNotFoundError) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        peerRepo.existsByPeerId[1] = false;
        UserService service(peerRepo, networkRepo, membership);

        const auto networkId = service.createNetwork(1, "n", "p");

        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::UserServiceError>(networkId));
        EXPECT_EQ(std::get<vpsm::server::port::UserServiceError>(networkId), vpsm::server::port::UserServiceError::PeerNotFound);
    }

    TEST(UserServiceTest, deleteNetwork_requesterNeOwner_ForbiddenError) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        networkRepo.networksById.emplace(42, makeNetwork(42, 100, "pass"));
        UserService service(peerRepo, networkRepo, membership);

        const auto ok = service.deleteNetwork(200, 42);

        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::UserServiceError>(ok));
        EXPECT_EQ(std::get<vpsm::server::port::UserServiceError>(ok), vpsm::server::port::UserServiceError::Forbidden);
    }

    TEST(UserServiceTest, joinNetwork_neverniyParol_InvalidPasswordError) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        peerRepo.existsByPeerId[9] = true;
        networkRepo.networksById.emplace(5, makeNetwork(5, 9, "correct"));
        UserService service(peerRepo, networkRepo, membership);

        const auto vip = service.joinNetwork(9, 5, "wrong");

        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::UserServiceError>(vip));
        EXPECT_EQ(std::get<vpsm::server::port::UserServiceError>(vip), vpsm::server::port::UserServiceError::InvalidPassword);
    }

    TEST(UserServiceTest, joinNetwork_peerUzheVSeti_SushestvuyushiyVip) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        peerRepo.existsByPeerId[9] = true;
        networkRepo.networksById.emplace(5, makeNetwork(5, 9, "pass"));
        const auto key = (static_cast<std::uint64_t>(5) << 32) | 9ull;
        membership.hasPeerByKey[key] = true;
        membership.resolveVipByKey[key] = 777;
        UserService service(peerRepo, networkRepo, membership);

        const auto vip = service.joinNetwork(9, 5, "pass");

        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::JoinNetworkSuccess>(vip));
        EXPECT_EQ(std::get<vpsm::server::port::JoinNetworkSuccess>(vip).vip, 777u);
        EXPECT_EQ(membership.allocateVipCalls, 0);
    }

    TEST(UserServiceTest, leaveNetwork_peerNeSushestvuet_PeerNotFoundError) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        peerRepo.existsByPeerId[11] = false;
        networkRepo.existsByNetworkId[1] = true;
        UserService service(peerRepo, networkRepo, membership);

        const auto ok = service.leaveNetwork(11, 1);

        ASSERT_TRUE(std::holds_alternative<vpsm::server::port::UserServiceError>(ok));
        EXPECT_EQ(std::get<vpsm::server::port::UserServiceError>(ok), vpsm::server::port::UserServiceError::PeerNotFound);
    }

    TEST(UserServiceTest, leaveNetwork_uspeshniyVihod_True) {

        PeerRepositoryFake peerRepo;
        NetworkRepositoryFake networkRepo;
        MembershipStoreFake membership;
        peerRepo.existsByPeerId[11] = true;
        networkRepo.existsByNetworkId[1] = true;
        const auto key = (static_cast<std::uint64_t>(1) << 32) | 11ull;
        membership.hasPeerByKey[key] = true;
        membership.releaseVipResult = true;
        UserService service(peerRepo, networkRepo, membership);

        const auto ok = service.leaveNetwork(11, 1);

        EXPECT_TRUE(std::holds_alternative<vpsm::server::port::ActionSuccess>(ok));
        EXPECT_EQ(membership.releaseVipCalls, 1);
        EXPECT_EQ(membership.lastNetworkId, 1u);
        EXPECT_EQ(membership.lastPeerId, 11u);
    }

}
