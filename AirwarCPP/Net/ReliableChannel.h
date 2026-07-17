#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

// 6-byte packet header
struct PacketHeader {
    uint8_t flags = 0;
    uint16_t seq = 0;
    uint16_t ack = 0;
    uint8_t ackBits = 0;

    static constexpr uint8_t FLAG_RELIABLE = 0x01;
    static constexpr uint8_t FLAG_ACK      = 0x02;
    static constexpr uint8_t FLAG_FRAGMENT = 0x04;

    bool isReliable() const { return flags & FLAG_RELIABLE; }
    bool isAck() const { return flags & FLAG_ACK; }
    bool isFragment() const { return flags & FLAG_FRAGMENT; }

    void setReliable() { flags |= FLAG_RELIABLE; }
    void setAck() { flags |= FLAG_ACK; }
    void setFragment() { flags |= FLAG_FRAGMENT; }

    std::vector<uint8_t> encode() const {
        return {flags, (uint8_t)(seq & 0xFF), (uint8_t)((seq >> 8) & 0xFF),
                (uint8_t)(ack & 0xFF), (uint8_t)((ack >> 8) & 0xFF), ackBits};
    }

    static PacketHeader decode(const uint8_t* data) {
        PacketHeader h;
        h.flags = data[0];
        h.seq = data[1] | ((uint16_t)data[2] << 8);
        h.ack = data[3] | ((uint16_t)data[4] << 8);
        h.ackBits = data[5];
        return h;
    }
};

struct ReliableChannel {
    uint16_t nextSeq = 0;
    uint16_t expectedSeq = 0;
    static constexpr int WINDOW_SIZE = 32;

    struct PendingPacket {
        std::vector<uint8_t> data;
        int retries = 0;
        uint64_t sentTime = 0;
    };
    std::map<uint16_t, PendingPacket> sendWindow;

    std::function<void(const std::vector<uint8_t>&)> sendCallback;

    explicit ReliableChannel(std::function<void(const std::vector<uint8_t>&)> cb)
        : sendCallback(std::move(cb)) {}

    std::vector<uint8_t> makePacket(const std::vector<uint8_t>& payload, bool reliable) {
        PacketHeader h;
        h.seq = nextSeq;
        if (reliable) {
            h.setReliable();
            auto hdr = h.encode();
            std::vector<uint8_t> pkt;
            pkt.insert(pkt.end(), hdr.begin(), hdr.end());
            pkt.insert(pkt.end(), payload.begin(), payload.end());
            sendWindow[nextSeq] = {pkt, 0, nowMs()};
            ++nextSeq;
            return pkt;
        } else {
            auto hdr = h.encode();
            std::vector<uint8_t> pkt;
            pkt.insert(pkt.end(), hdr.begin(), hdr.end());
            pkt.insert(pkt.end(), payload.begin(), payload.end());
            return pkt;
        }
    }

    void send(const std::vector<uint8_t>& payload, bool reliable) {
        auto pkt = makePacket(payload, reliable);
        if (sendCallback) sendCallback(pkt);
    }

    // Process incoming packet; returns payload if this is new data, empty if ACK-only or duplicate
    std::vector<uint8_t> receive(const uint8_t* data, size_t len) {
        if (len < 6) return {};
        auto hdr = PacketHeader::decode(data);
        std::vector<uint8_t> payload(data + 6, data + len);

        if (hdr.isReliable()) {
            // Send ACK
            PacketHeader ackHdr;
            ackHdr.setAck();
            ackHdr.ack = hdr.seq;
            auto ackPkt = ackHdr.encode();
            if (sendCallback) sendCallback(ackPkt);

            // Deliver only if in order
            if (hdr.seq == expectedSeq) {
                ++expectedSeq;
                return payload;
            }
            return {};  // out of order or duplicate
        }

        if (hdr.isAck()) {
            // Remove acknowledged packet from send window
            sendWindow.erase(hdr.ack);
            return {};
        }

        return payload;
    }

    void update() {
        uint64_t now = nowMs();
        for (auto it = sendWindow.begin(); it != sendWindow.end(); ) {
            auto& pp = it->second;
            if (pp.retries < 5 && now - pp.sentTime > 100 * (1 << pp.retries)) {
                ++pp.retries;
                pp.sentTime = now;
                if (sendCallback) sendCallback(pp.data);
            }
            ++it;
        }
    }

    int pendingCount() const { return (int)sendWindow.size(); }

private:
    static uint64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
