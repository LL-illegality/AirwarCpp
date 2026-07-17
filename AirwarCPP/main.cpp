#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include "Core/RNG.h"
#include "Net/UdpSocket.h"
#include "Net/ReliableChannel.h"
#include "Net/ConnectionManager.h"

static int testsPassed = 0;
static int testsFailed = 0;
#define CHECK(cond, msg) do {                                              \
    if (cond) { ++testsPassed;                                              \
        printf("  PASS: %s\n", msg);                                        \
    } else { ++testsFailed;                                                 \
        printf("  FAIL: %s\n  at line %d\n", msg, __LINE__);                \
    } } while(0)

int main(int, char**) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("AirwarCPP Phase 8 -- UDP Transport Layer\n");
    printf("========================================\n\n");

    seedRNG();

    /* ====== 1. PacketHeader encode/decode ====== */
    printf("[1] PacketHeader encode/decode\n");
    {
        PacketHeader h;
        h.flags = PacketHeader::FLAG_RELIABLE | PacketHeader::FLAG_FRAGMENT;
        h.seq = 0x1234;
        h.ack = 0xABCD;
        h.ackBits = 0x7F;

        CHECK(h.isReliable(), "FLAG_RELIABLE set");
        CHECK(!h.isAck(), "FLAG_ACK not set");
        CHECK(h.isFragment(), "FLAG_FRAGMENT set");

        auto bytes = h.encode();
        CHECK(bytes.size() == 6, "Header encoded to 6 bytes");

        auto h2 = PacketHeader::decode(bytes.data());
        CHECK(h2.flags == h.flags, "Decoded flags match");
        CHECK(h2.seq == 0x1234, "Decoded seq match");
        CHECK(h2.ack == 0xABCD, "Decoded ack match");
        CHECK(h2.ackBits == 0x7F, "Decoded ackBits match");
    }
    {
        PacketHeader h;
        h.setReliable();
        CHECK(h.isReliable() && !h.isAck(), "setReliable");
        h.setAck();
        CHECK(h.isReliable() && h.isAck(), "setAck preserves reliable");
        h.setFragment();
        CHECK(h.isFragment(), "setFragment");
    }

    /* ====== 2. ReliableChannel: unreliable packets ====== */
    printf("[2] ReliableChannel unreliable packets\n");
    {
        int sendCount = 0;
        ReliableChannel ch([&](const std::vector<uint8_t>&) { ++sendCount; });

        std::vector<uint8_t> payload = {'H', 'e', 'l', 'l', 'o'};
        ch.send(payload, false);
        CHECK(sendCount == 1, "Unreliable send calls callback");
        CHECK(ch.pendingCount() == 0, "No pending for unreliable");
    }

    /* ====== 3. ReliableChannel: reliable packets ====== */
    printf("[3] ReliableChannel reliable packets\n");
    {
        std::vector<std::vector<uint8_t>> sent;
        ReliableChannel ch([&](const std::vector<uint8_t>& pkt) {
            sent.push_back(pkt);
        });

        std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
        ch.send(payload, true);
        CHECK(sent.size() == 1, "Reliable send produces packet");
        CHECK(ch.pendingCount() == 1, "Reliable tracked in sendWindow");

        auto& pkt = sent[0];
        CHECK(pkt.size() == 6 + 5, "Packet = 6 header + 5 payload");

        auto hdr = PacketHeader::decode(pkt.data());
        CHECK(hdr.isReliable(), "Reliable flag set");
        CHECK(hdr.seq == 0, "First seq == 0");

        std::vector<uint8_t> recvPayload(pkt.begin() + 6, pkt.end());
        CHECK(recvPayload.size() == 5, "Payload length correct");
    }

    /* ====== 4. ReliableChannel: ACK processing ====== */
    printf("[4] ReliableChannel ACK\n");
    {
        std::vector<std::vector<uint8_t>> sent;
        ReliableChannel ch([&](const std::vector<uint8_t>& pkt) { sent.push_back(pkt); });

        ch.send({10, 20, 30}, true);
        CHECK(ch.pendingCount() == 1, "Pending before ACK");

        // Craft an ACK for seq 0
        PacketHeader ackH;
        ackH.setAck();
        ackH.ack = 0;
        auto ackPkt = ackH.encode();

        auto result = ch.receive(ackPkt.data(), ackPkt.size());
        CHECK(result.empty(), "ACK returns empty payload");
        CHECK(ch.pendingCount() == 0, "Pending cleared after ACK");
    }

    /* ====== 5. ReliableChannel: receive in-order ====== */
    printf("[5] ReliableChannel in-order delivery\n");
    {
        ReliableChannel ch(nullptr);
        // Simulate receiving 3 reliable packets with seq 0, 1, 2
        for (uint16_t seq = 0; seq < 3; ++seq) {
            PacketHeader h;
            h.setReliable();
            h.seq = seq;
            auto pkt = h.encode();
            pkt.push_back((uint8_t)('A' + seq));
            auto result = ch.receive(pkt.data(), pkt.size());
            CHECK(!result.empty() && result[0] == (uint8_t)('A' + seq),
                  ("In-order delivery seq=" + std::to_string(seq)).c_str());
        }
    }

    /* ====== 6. ReliableChannel: out-of-order rejection ====== */
    printf("[6] ReliableChannel out-of-order\n");
    {
        ReliableChannel ch(nullptr);
        // Send seq 1 before seq 0
        PacketHeader h;
        h.setReliable();
        h.seq = 1;
        auto pkt = h.encode();
        pkt.push_back('B');
        auto result = ch.receive(pkt.data(), pkt.size());
        CHECK(result.empty(), "Out-of-order seq 1 rejected");
    }

    /* ====== 7. ReliableChannel: retransmit ====== */
    printf("[7] ReliableChannel retransmit\n");
    {
        int sendCount = 0;
        ReliableChannel ch([&](const std::vector<uint8_t>&) { ++sendCount; });

        ch.send({1, 2, 3}, true);
        CHECK(sendCount == 1, "Initial send");

        // Simulate time passing and update triggering retransmit
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        ch.update();
        CHECK(sendCount >= 2, "Retransmit triggered after timeout");
    }

    /* ====== 8. ConnectionManager: create and track ====== */
    printf("[8] ConnectionManager basics\n");
    {
        ConnectionManager cm;
        CHECK(cm.count() == 0, "Empty initially");

        sockaddr_storage addr1{}, addr2{};
        UdpSocket::makeAddr("127.0.0.1", 8000, addr1);
        UdpSocket::makeAddr("127.0.0.1", 8001, addr2);

        uint32_t id1 = cm.createConnection(addr1);
        uint32_t id2 = cm.createConnection(addr2);
        CHECK(cm.count() == 2, "Two connections created");
        CHECK(id1 != id2, "Unique IDs assigned");

        auto* conn1 = cm.get(id1);
        CHECK(conn1 != nullptr, "Connection found by ID");
        CHECK(conn1->state == ConnectionState::CONNECTING, "State == CONNECTING");
        CHECK(conn1->playerId == -1, "PlayerId == -1 initially");

        cm.setConnected(id1);
        CHECK(conn1->state == ConnectionState::CONNECTED, "State == CONNECTED after setConnected");

        auto* found = cm.findByAddr(addr1);
        CHECK(found != nullptr && found->clientId == id1, "findByAddr works");
        CHECK(cm.findByAddr(addr2) != nullptr, "findByAddr for addr2");
    }

    /* ====== 9. ConnectionManager: remove and timeout ====== */
    printf("[9] ConnectionManager timeout/remove\n");
    {
        ConnectionManager cm;
        sockaddr_storage addr;
        UdpSocket::makeAddr("127.0.0.1", 9000, addr);
        uint32_t id = cm.createConnection(addr);

        CHECK(cm.count() == 1, "One connection");
        cm.remove(id);
        CHECK(cm.count() == 0, "Removed connection");
        CHECK(cm.get(id) == nullptr, "Null after removal");
    }
    {
        ConnectionManager cm;
        sockaddr_storage addr;
        UdpSocket::makeAddr("10.0.0.1", 5000, addr);
        auto id = cm.createConnection(addr);

        // Manually simulate timeout by setting lastRecvTime far in the past
        auto* conn = cm.get(id);
        conn->lastRecvTime = 1;  // year 1970
        int timeoutCount = 0;
        cm.setTimeoutCallback([&](uint32_t) { ++timeoutCount; return true; });
        cm.checkTimeouts(100000);
        CHECK(cm.count() == 0, "Timed out connection removed");
        CHECK(timeoutCount == 1, "Timeout callback fired exactly once");
    }

    /* ====== 10. ConnectionManager: active count ====== */
    printf("[10] ConnectionManager active count\n");
    {
        ConnectionManager cm;
        sockaddr_storage addr;
        UdpSocket::makeAddr("192.168.1.1", 8000, addr);
        uint32_t id = cm.createConnection(addr);
        CHECK(cm.activeCount() == 0, "Not active while CONNECTING");
        cm.setConnected(id);
        CHECK(cm.activeCount() == 1, "Active after setConnected");
        cm.remove(id);
        CHECK(cm.activeCount() == 0, "Active count 0 after remove");
    }

    /* ====== 11. PacketHeader round-trip stress ====== */
    printf("[11] PacketHeader stress\n");
    {
        for (int i = 0; i < 5; ++i) {
            PacketHeader h;
            h.flags = (uint8_t)(rand() & 7);
            h.seq = (uint16_t)(rand() & 0xFFFF);
            h.ack = (uint16_t)(rand() & 0xFFFF);
            h.ackBits = (uint8_t)(rand() & 0xFF);
            auto bytes = h.encode();
            auto h2 = PacketHeader::decode(bytes.data());
            CHECK(h.flags == h2.flags && h.seq == h2.seq && h.ack == h2.ack && h.ackBits == h2.ackBits,
                  ("Random round-trip #" + std::to_string(i)).c_str());
        }
    }

    /* ====== 12. ReliableChannel: sequence number wrapping ====== */
    printf("[12] ReliableChannel seq wrap\n");
    {
        ReliableChannel ch(nullptr);
        ch.nextSeq = 0xFFF0;
        for (int i = 0; i < 5; ++i) {
            std::vector<uint8_t> payload = {(uint8_t)i};
            ch.send(payload, true);
        }
        CHECK(ch.pendingCount() == 5, "5 packets pending after near-wrap");
    }

    /* ====== 13. ConnectionManager: clear ====== */
    printf("[13] ConnectionManager clear\n");
    {
        ConnectionManager cm;
        sockaddr_storage a1, a2;
        UdpSocket::makeAddr("10.0.0.1", 1, a1);
        UdpSocket::makeAddr("10.0.0.2", 2, a2);
        cm.createConnection(a1);
        cm.createConnection(a2);
        CHECK(cm.count() == 2, "2 connections before clear");
        cm.clear();
        CHECK(cm.count() == 0, "0 connections after clear");
    }

    /* ====== 14. Address utility ====== */
    printf("[14] Address utility\n");
    {
        sockaddr_storage addr;
        CHECK(UdpSocket::makeAddr("127.0.0.1", 8080, addr), "makeAddr IPv4");
        std::string ip; int port;
        UdpSocket::addrToString(addr, ip, port);
        CHECK(ip == "127.0.0.1" && port == 8080, "IPv4 round-trip");

        CHECK(UdpSocket::makeAddr("::1", 8080, addr), "makeAddr IPv6");
        UdpSocket::addrToString(addr, ip, port);
        CHECK(ip == "::1" && port == 8080, "IPv6 round-trip");
    }

    /* ====== Summary ====== */
    int total = testsPassed + testsFailed;
    printf("\n========================================\n");
    printf("  Results: %d / %d passed, %d failed\n",
           testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
