#include "PacketCodecV2.hpp"

#include <QtEndian>
#include <openssl/evp.h>

#include <memory>
#include <algorithm>

namespace vpsm::client {
    namespace {
        constexpr int outerSize = 17;
        constexpr int innerSize = 13;
        constexpr int tagSize = 16;
        constexpr quint8 version = 2;
        using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

        QByteArray nonceFor(quint32 direction, quint64 sequence) {
            QByteArray nonce(12, Qt::Uninitialized);
            auto* bytes = reinterpret_cast<uchar*>(nonce.data());
            qToBigEndian<quint32>(direction, bytes);
            qToBigEndian<quint64>(sequence, bytes + 4);
            return nonce;
        }

        std::optional<QByteArray> encrypt(
            const QByteArray& aad,
            const QByteArray& plaintext,
            const QByteArray& key,
            quint32 direction,
            quint64 sequence
        ) {
            if (key.size() != 32) return std::nullopt;
            const auto nonce = nonceFor(direction, sequence);
            CipherContext ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
            if (!ctx) return std::nullopt;
            if (EVP_EncryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
                EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), nullptr) != 1 ||
                EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr,
                    reinterpret_cast<const uchar*>(key.constData()),
                    reinterpret_cast<const uchar*>(nonce.constData())) != 1) return std::nullopt;

            int written = 0;
            if (EVP_EncryptUpdate(ctx.get(), nullptr, &written,
                    reinterpret_cast<const uchar*>(aad.constData()), aad.size()) != 1) return std::nullopt;
            QByteArray result(plaintext.size() + tagSize, Qt::Uninitialized);
            int total = 0;
            if (EVP_EncryptUpdate(ctx.get(), reinterpret_cast<uchar*>(result.data()), &written,
                    reinterpret_cast<const uchar*>(plaintext.constData()), plaintext.size()) != 1) return std::nullopt;
            total += written;
            if (EVP_EncryptFinal_ex(ctx.get(), reinterpret_cast<uchar*>(result.data()) + total, &written) != 1) return std::nullopt;
            total += written;
            if (total != plaintext.size() ||
                EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_GET_TAG, tagSize, result.data() + total) != 1) return std::nullopt;
            return result;
        }

        std::optional<QByteArray> decrypt(
            const QByteArray& aad,
            const QByteArray& cipherAndTag,
            const QByteArray& key,
            quint32 direction,
            quint64 sequence
        ) {
            if (key.size() != 32 || cipherAndTag.size() < innerSize + tagSize) return std::nullopt;
            const int cipherSize = cipherAndTag.size() - tagSize;
            const auto nonce = nonceFor(direction, sequence);
            CipherContext ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
            if (!ctx) return std::nullopt;
            if (EVP_DecryptInit_ex(ctx.get(), EVP_chacha20_poly1305(), nullptr, nullptr, nullptr) != 1 ||
                EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_IVLEN, nonce.size(), nullptr) != 1 ||
                EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr,
                    reinterpret_cast<const uchar*>(key.constData()),
                    reinterpret_cast<const uchar*>(nonce.constData())) != 1) return std::nullopt;

            int written = 0;
            if (EVP_DecryptUpdate(ctx.get(), nullptr, &written,
                    reinterpret_cast<const uchar*>(aad.constData()), aad.size()) != 1) return std::nullopt;
            QByteArray plaintext(cipherSize, Qt::Uninitialized);
            int total = 0;
            if (EVP_DecryptUpdate(ctx.get(), reinterpret_cast<uchar*>(plaintext.data()), &written,
                    reinterpret_cast<const uchar*>(cipherAndTag.constData()), cipherSize) != 1) return std::nullopt;
            total += written;
            if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_AEAD_SET_TAG, tagSize,
                    const_cast<char*>(cipherAndTag.constData() + cipherSize)) != 1 ||
                EVP_DecryptFinal_ex(ctx.get(), reinterpret_cast<uchar*>(plaintext.data()) + total, &written) != 1) return std::nullopt;
            total += written;
            plaintext.resize(total);
            return plaintext;
        }
    }

    std::optional<QByteArray> PacketCodecV2::encodeClientPacket(
        const Session& session, quint64 sequence, PacketType type, quint32 networkId,
        quint32 sourceVip, quint32 destinationVip, const QByteArray& payload
    ) {
        return encode(session, sequence, type, networkId, sourceVip, destinationVip,
                      payload, Direction::ClientToRouter);
    }

    std::optional<QByteArray> PacketCodecV2::encodeRouterPacket(
        const Session& session, quint64 sequence, PacketType type, quint32 networkId,
        quint32 sourceVip, quint32 destinationVip, const QByteArray& payload
    ) {
        return encode(session, sequence, type, networkId, sourceVip, destinationVip,
                      payload, Direction::RouterToClient);
    }

    std::optional<DecodedPacket> PacketCodecV2::decodeRouterPacket(
        const Session& session, const QByteArray& datagram
    ) {
        return decode(session, datagram, Direction::RouterToClient);
    }

    std::optional<DecodedPacket> PacketCodecV2::decodeClientPacket(
        const Session& session, const QByteArray& datagram
    ) {
        return decode(session, datagram, Direction::ClientToRouter);
    }

    std::optional<QByteArray> PacketCodecV2::encode(
        const Session& session, quint64 sequence, PacketType type, quint32 networkId,
        quint32 sourceVip, quint32 destinationVip, const QByteArray& payload, Direction direction
    ) {
        if (!session.isValid() || sequence == 0 || networkId == 0 ||
            payload.size() > MaximumPayloadSize) return std::nullopt;
        const auto typeValue = static_cast<quint8>(type);
        if (typeValue > static_cast<quint8>(PacketType::Keepalive)) return std::nullopt;

        QByteArray outer(outerSize, Qt::Uninitialized);
        auto* outerBytes = reinterpret_cast<uchar*>(outer.data());
        outerBytes[0] = version;
        qToBigEndian<quint64>(session.sessionId, outerBytes + 1);
        qToBigEndian<quint64>(sequence, outerBytes + 9);

        QByteArray plaintext(innerSize + payload.size(), Qt::Uninitialized);
        auto* inner = reinterpret_cast<uchar*>(plaintext.data());
        inner[0] = typeValue;
        qToBigEndian<quint32>(networkId, inner + 1);
        qToBigEndian<quint32>(sourceVip, inner + 5);
        qToBigEndian<quint32>(destinationVip, inner + 9);
        if (!payload.isEmpty()) std::copy(payload.cbegin(), payload.cend(), plaintext.begin() + innerSize);

        const auto encrypted = encrypt(outer, plaintext, session.dataPlaneKey,
                                       static_cast<quint32>(direction), sequence);
        if (!encrypted) return std::nullopt;
        return outer + *encrypted;
    }

    std::optional<DecodedPacket> PacketCodecV2::decode(
        const Session& session, const QByteArray& datagram, Direction direction
    ) {
        if (!session.isValid() || datagram.size() < outerSize + innerSize + tagSize ||
            datagram.size() > MaximumDatagramSize) return std::nullopt;
        const auto* outer = reinterpret_cast<const uchar*>(datagram.constData());
        if (outer[0] != version || qFromBigEndian<quint64>(outer + 1) != session.sessionId) return std::nullopt;
        const auto sequence = qFromBigEndian<quint64>(outer + 9);
        if (sequence == 0) return std::nullopt;
        const auto plaintext = decrypt(datagram.left(outerSize), datagram.mid(outerSize),
                                       session.dataPlaneKey, static_cast<quint32>(direction), sequence);
        if (!plaintext || plaintext->size() < innerSize) return std::nullopt;
        const auto* inner = reinterpret_cast<const uchar*>(plaintext->constData());
        if (inner[0] > static_cast<quint8>(PacketType::Keepalive)) return std::nullopt;
        return DecodedPacket{
            .sequence = sequence,
            .type = static_cast<PacketType>(inner[0]),
            .networkId = qFromBigEndian<quint32>(inner + 1),
            .sourceVip = qFromBigEndian<quint32>(inner + 5),
            .destinationVip = qFromBigEndian<quint32>(inner + 9),
            .payload = plaintext->mid(innerSize),
        };
    }
}