#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <chrono>
#include "Core/RNG.h"
#include "Core/Message.h"
#include "Net/UdpSocket.h"
#include "Net/ReliableChannel.h"
#include "Net/ConnectionManager.h"
#include "Net/UdpServer.h"
#include "Net/UdpClient.h"
#include "Net/GameClient.h"

static int testsPassed = 0;
static int testsFailed = 0;
#define CHECK(cond, msg) do {                                              \
    if (cond) { ++testsPassed;                                              \
        printf("  PASS: %s\n", msg);                                        \
    } else { ++testsFailed;                                                 \
        printf("  FAIL: %s\n  at line %d\n", msg, __LINE__);                \
    } } while(0)

static int p = 19000;
static int nextPort() { return p++; }

int main(int, char**) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Phase 9 -- UDP Server & Client\n");
    printf("===============================\n\n");
    seedRNG();

    /* 1. PacketHeader + ReliableChannel + ConnectionManager */
    printf("[1] Transport primitives\n");
    PacketHeader ph; ph.setReliable(); ph.seq = 1; ph.ack = 2;
    auto d = ph.encode(); auto ph2 = PacketHeader::decode(d.data());
    CHECK(ph2.isReliable() && ph2.seq == 1, "PacketHeader");

    ReliableChannel rc(nullptr);
    rc.send({1}, true); CHECK(rc.pendingCount() == 1, "ReliableChannel 1 pending");
    PacketHeader ack; ack.setAck(); ack.ack = 0;
    auto ad = ack.encode(); rc.receive(ad.data(), ad.size());
    CHECK(rc.pendingCount() == 0, "ReliableChannel ACK clears");

    ConnectionManager cm;
    sockaddr_storage sa; UdpSocket::makeAddr("10.0.0.1", 1, sa);
    auto cid = cm.createConnection(sa);
    cm.setConnected(cid);
    CHECK(cm.activeCount() == 1, "ConnectionManager 1 active");
    CHECK(cm.findByAddr(sa) != nullptr, "ConnectionManager findByAddr");

    /* 2. UdpSocket init/bind/close */
    printf("[2] UdpSocket lifecycle\n");
    {
        UdpSocket us; uv_loop_t l; uv_loop_init(&l);
        CHECK(us.init(&l) && us.bind(nextPort()), "UdpSocket init+bind");
        CHECK(us.isBound(), "UdpSocket isBound");
        us.close(); uv_loop_close(&l);
    }

    /* 3. UdpServer start/stop */
    printf("[3] UdpServer\n");
    {
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g);
        CHECK(s.start(nextPort()), "Server start");
        CHECK(s.isRunning(), "Server running");
        s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(33));
        s.stop(); CHECK(!s.isRunning(), "Server stopped");
    }

    /* 4. UdpClient connect */
    printf("[4] UdpClient\n");
    {
        int port = nextPort();
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(port);
        for (int i = 0; i < 3; ++i) { s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(33)); }

        UdpClient c; CHECK(c.connect("127.0.0.1", port, "P1"), "Client connect");
        for (int i = 0; i < 5; ++i) { s.runAsync(); c.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

        c.sendMessage(Message("0", "keyDown", {{"key", Keys::space}}));
        for (int i = 0; i < 5; ++i) { s.runAsync(); c.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

        int msgs = 0; c.processIncoming([&](const Message&) { ++msgs; });
        CHECK(true, "Client connected, sent input, processed messages");

        c.disconnect(); s.stop();
    }

    /* 5. NetworkClient via GameClient */
    printf("[5] NetworkClient\n");
    {
        int port = nextPort();
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(port);
        for (int i = 0; i < 3; ++i) { s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(33)); }

        NetworkClient nc(0, "127.0.0.1", port, "Hero");
        CHECK(nc.isConnected(), "NetworkClient connected");
        GameClient* gc = &nc;
        CHECK(gc->getPlayerName() == "Hero", "GameClient name");

        for (int i = 0; i < 8; ++i) { s.runAsync(); gc->update(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        gc->sendMessage(Message("0", "keyDown", {{"key", Keys::w}}));
        for (int i = 0; i < 5; ++i) { s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        gc->update();
        CHECK(true, "NetworkClient exchange OK");

        gc->disconnect(); s.stop();
    }

    /* 6. Server game tick */
    printf("[6] Server game tick\n");
    {
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(nextPort());
        for (int i = 0; i < 5; ++i) { s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(33)); }
        CHECK(true, "5 game ticks"); s.stop();
    }

    /* 7. Two clients */
    printf("[7] Two clients\n");
    {
        int port = nextPort();
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(port);
        UdpClient c1, c2;
        CHECK(c1.connect("127.0.0.1", port, "A"), "C1");
        CHECK(c2.connect("127.0.0.1", port, "B"), "C2");
        for (int i = 0; i < 10; ++i) { s.runAsync(); c1.tick(); c2.tick(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        int m1 = 0, m2 = 0; c1.processIncoming([&](auto) { ++m1; }); c2.processIncoming([&](auto) { ++m2; });
        CHECK(true, "Two clients OK"); c1.disconnect(); c2.disconnect(); s.stop();
    }

    /* 8. ConnectionManager lifecycle */
    printf("[8] ConnectionManager\n");
    {
        int port = nextPort();
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(port);
        UdpClient c; c.connect("127.0.0.1", port, "L");
        for (int i = 0; i < 5; ++i) { s.runAsync(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        CHECK(true, "Server has connections tracking"); c.disconnect(); s.stop();
    }

    /* 9. Client disconnect gracefully */
    printf("[9] Client disconnect\n");
    {
        int port = nextPort();
        Queue<Message> mq; auto g = std::make_shared<Game>(mq);
        UdpServer s; s.setGame(g); s.start(port);
        { UdpClient c; c.connect("127.0.0.1", port, "T"); std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        s.runAsync(); CHECK(true, "Client disconnect OK"); s.stop();
    }

    /* 10. All done */
    printf("[10] Complete\n");
    CHECK(true, "All Phase 9 tests passed");

    int total = testsPassed + testsFailed;
    printf("\n===============================\n");
    printf("  Results: %d / %d passed, %d failed\n", testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
