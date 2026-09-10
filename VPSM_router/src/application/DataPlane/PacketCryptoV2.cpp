#include "PacketCryptoV2.hpp"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <limits>
#include <memory>

namespace vpsm::server::application {
    namespace {
        using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

        std::array<std::uint8_t, 12> makeNonce(
            PacketCryptoV2::Direction direction,
            std::uint64_t seq
        ) {
            std::array<std::uint8_t, 12> nonce{};
            const auto prefix = static_cast<std::uint32_t>(direction);
            for (int i = 0; i < 4; ++i) {
                nonce[i] = static_cast<std::uint8_t>(prefix >> ((3 - i) * 8));
            }
            for (int i = 0; i < 8; ++i) {
                nonce[4 + i] = static_cast<std::uint8_t>(seq >> ((7 - i) * 8));
            }
            return nonce;
        }

        bool fitsOpenSslInt(std::size_t size) {
            return size <= static_cast<std::size_t>(std::numeric_limits<int>::max());
        }
    }

    std::array<std::uint8_t, domain::OUTER_HEADER_V2_SIZE> PacketCryptoV2::encodeOuter(
        const domain::OutPacketHeaderV2& outer
    ) {
        std::array<std::uint8_t, domain::OUTER_HEADER_V2_SIZE> encoded{};
        encoded[0] = outer.packetVersion;
        for (int i = 0; i < 8; ++i) {
            encoded[1 + i] = static_cast<std::uint8_t>(outer.sessionId >> ((7 - i) * 8));
            encoded[9 + i] = static_cast<std::uint8_t>(outer.seq >> ((7 - i) * 8));
        }
        return encoded;
    }

    std::optional<std::vector<std::uint8_t>> PacketCryptoV2::encrypt(
        const domain::OutPacketHeaderV2& outer,
        const std::vector<std::uint8_t>& plaintext,
        const Key& key,
        Direction direction
    ) {
        if (outer.packetVersion != domain::VERSION_V2 || !fitsOpenSslInt(plaintext.size())) {
            return std::nullopt;
        }

        const auto aad = encodeOuter(outer);
        const auto nonce = makeNonce(direction, outer.seq);
        CipherContext ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
        if (!ctx) return std::nullopt;

        if (EVP_EncryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1 ||
            EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) != 1) {
            return std::nullopt;
        }

        int written = 0;
        if (EVP_EncryptUpdate(ctx.get(), nullptr, &written, aad.data(), static_cast<int>(aad.size())) != 1) {
            return std::nullopt;
        }

        std::vector<std::uint8_t> out(aad.size() + plaintext.size() + domain::AUTH_TAG_V2_SIZE);
        std::copy(aad.begin(), aad.end(), out.begin());
        int cipherSize = 0;
        if (EVP_EncryptUpdate(
                ctx.get(), out.data() + aad.size(), &written,
                plaintext.data(), static_cast<int>(plaintext.size())) != 1) {
            return std::nullopt;
        }
        cipherSize += written;
        if (EVP_EncryptFinal_ex(ctx.get(), out.data() + aad.size() + cipherSize, &written) != 1) {
            return std::nullopt;
        }
        cipherSize += written;
        if (static_cast<std::size_t>(cipherSize) != plaintext.size()) return std::nullopt;

        if (EVP_CIPHER_CTX_ctrl(
                ctx.get(), EVP_CTRL_AEAD_GET_TAG, domain::AUTH_TAG_V2_SIZE,
                out.data() + aad.size() + plaintext.size()) != 1) {
            return std::nullopt;
        }
        return out;
    }

    std::optional<std::vector<std::uint8_t>> PacketCryptoV2::decrypt(
        const std::uint8_t* packet,
        std::size_t packetSize,
        const Key& key,
        Direction direction
    ) {
        const auto minimum = domain::OUTER_HEADER_V2_SIZE + domain::INNER_HEADER_V2_SIZE + domain::AUTH_TAG_V2_SIZE;
        if (packet == nullptr || packetSize < minimum) return std::nullopt;

        const std::size_t cipherSize = packetSize - domain::OUTER_HEADER_V2_SIZE - domain::AUTH_TAG_V2_SIZE;
        if (!fitsOpenSslInt(cipherSize)) return std::nullopt;

        std::uint64_t seq = 0;
        for (std::size_t i = 9; i < 17; ++i) seq = (seq << 8) | packet[i];
        const auto nonce = makeNonce(direction, seq);
        CipherContext ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
        if (!ctx) return std::nullopt;

        if (EVP_DecryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
            EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1 ||
            EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, key.data(), nonce.data()) != 1) {
            return std::nullopt;
        }

        int written = 0;
        if (EVP_DecryptUpdate(
                ctx.get(), nullptr, &written, packet,
                static_cast<int>(domain::OUTER_HEADER_V2_SIZE)) != 1) {
            return std::nullopt;
        }

        std::vector<std::uint8_t> plaintext(cipherSize);
        int plainSize = 0;
        if (EVP_DecryptUpdate(
                ctx.get(), plaintext.data(), &written,
                packet + domain::OUTER_HEADER_V2_SIZE, static_cast<int>(cipherSize)) != 1) {
            return std::nullopt;
        }
        plainSize += written;

        auto* tag = const_cast<std::uint8_t*>(packet + domain::OUTER_HEADER_V2_SIZE + cipherSize);
        if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, domain::AUTH_TAG_V2_SIZE, tag) != 1 ||
            EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + plainSize, &written) != 1) {
            return std::nullopt;
        }
        plainSize += written;
        plaintext.resize(static_cast<std::size_t>(plainSize));
        return plaintext;
    }
}