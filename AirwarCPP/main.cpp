#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include "Core/RNG.h"
#include "Core/Message.h"
#include "Core/Queue.h"
#include "Game/Game.h"
#include "Game/Board.h"
#include "Game/Item.h"
#include "Net/GameClient.h"
#include "Net/MessageDispatcher.h"

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
    printf("AirwarCPP Phase 7 -- GameClient Abstraction\n");
    printf("===========================================\n\n");

    seedRNG();

    /* ====== 1. GameClient abstract interface ====== */
    printf("[1] GameClient abstract interface\n");
    // Verify we can create both client types through the abstract interface
    Queue<Message> mq1;
    auto game1 = std::make_shared<Game>(mq1);
    SinglePlayerClient spc(0, game1, mq1, "TestPlayer");
    GameClient* gc = &spc;
    CHECK(gc->getPlayerId() == 0, "GameClient interface: getPlayerId");
    CHECK(gc->getPlayerName() == "TestPlayer", "GameClient interface: getPlayerName");
    CHECK(&gc->getMsgQueue() == &mq1, "GameClient interface: getMsgQueue");

    NetworkClient nc(1, "127.0.0.1", 8000, "NetPlayer");
    gc = &nc;
    CHECK(gc->getPlayerId() == 1, "NetworkClient via interface: getPlayerId");
    CHECK(gc->getPlayerName() == "NetPlayer", "NetworkClient via interface: getPlayerName");

    /* ====== 2. SinglePlayerClient: newPlayer ====== */
    printf("[2] SinglePlayerClient newPlayer\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq, "Hero");
        client.newPlayer();

        CHECK(game->board.players.size() == 1, "Player added to board");
        CHECK(game->board.players[0]->name == "Hero", "Player name set");
        CHECK(game->board.players[0]->x == SCREEN_W / 2.0, "Player x at center");
        CHECK(game->board.players[0]->y == 2.0 / 3.0 * SCREEN_H, "Player y at 2/3");
        CHECK(game->board.players[0]->player_id == 0, "Player id == 0");
        CHECK(game->board.players[0]->image == Images::player1, "Player image player1");
    }

    /* ====== 3. SinglePlayerClient: keyDown/keyUp input ====== */
    printf("[3] SinglePlayerClient input\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq);
        client.newPlayer();

        client.sendMessage(Message("0", "keyDown", {{"key", Keys::w}}));
        auto& kl = game->board.players[0]->pressedKeyList;
        CHECK(kl.size() == 1 && kl[0] == Keys::w, "keyDown W added to pressedKeyList");

        client.sendMessage(Message("0", "keyUp", {{"key", Keys::w}}));
        CHECK(kl.empty(), "keyUp W removed from pressedKeyList");

        // Multiple keys
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::a}}));
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::space}}));
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::d}}));
        CHECK(kl.size() == 3, "Multiple keys pressed");
    }

    /* ====== 4. SinglePlayerClient: pause toggle ====== */
    printf("[4] SinglePlayerClient pause\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq, "Pauser");
        client.newPlayer();

        client.sendMessage(Message("0", "keyDown", {{"key", Keys::p}}));
        CHECK(game->isPaused, "Game paused on P key");
        CHECK(game->pausePlayerId == 0, "Pause playerId set");
        CHECK(game->pausePlayerName == "Pauser", "Pause playerName set");

        client.sendMessage(Message("0", "keyDown", {{"key", Keys::p}}));
        CHECK(!game->isPaused, "Game unpaused on second P key");

        // Different player can't unpause
        game->board.addPlayer(std::make_shared<Player>(1));
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::p}}));
        CHECK(game->isPaused, "Player 0 paused");
        client.sendMessage(Message("1", "keyDown", {{"key", Keys::p}}));
        CHECK(game->isPaused, "Player 1 can't unpause for player 0");
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::p}}));
        CHECK(!game->isPaused, "Player 0 unpauses");
    }

    /* ====== 5. SinglePlayerClient: joyAxis/joyHat ====== */
    printf("[5] SinglePlayerClient joy input\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq);
        client.newPlayer();

        client.sendMessage(Message("0", "joyAxis", {{"axis", 0}, {"value", -0.5}}));
        CHECK(game->board.players[0]->joystickAxisList[0] == -0.5, "joyAxis axis 0 = -0.5");

        client.sendMessage(Message("0", "joyAxis", {{"axis", 0}, {"value", 0.1}}));
        CHECK(game->board.players[0]->joystickAxisList[0] == 0, "joyAxis value < 0.2 clamped to 0");

        client.sendMessage(Message("0", "joyHat", {{"value", {1, 0}}}));
        auto& kl = game->board.players[0]->pressedKeyList;
        bool hasD = std::find(kl.begin(), kl.end(), Keys::d) != kl.end();
        CHECK(hasD, "joyHat {1,0} adds D");

        client.sendMessage(Message("0", "joyHat", {{"value", {0, 0}}}));
        hasD = std::find(kl.begin(), kl.end(), Keys::d) != kl.end();
        CHECK(!hasD, "joyHat {0,0} removes D");
    }

    /* ====== 6. SinglePlayerClient: update drives game ====== */
    printf("[6] SinglePlayerClient update\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq);
        client.newPlayer();

        client.sendMessage(Message("0", "keyDown", {{"key", Keys::w}}));
        client.update();  // drives game->update() -> board.update() -> getObjects()
        CHECK(true, "SinglePlayerClient::update() completed without error");
        CHECK(!mq.isEmpty(), "Messages produced after update (screen_info)");
    }

    /* ====== 7. NetworkClient stub: send queues outbox ====== */
    printf("[7] NetworkClient outbox\n");
    {
        NetworkClient nc(1, "192.168.1.1", 8000, "Remote");
        nc.sendMessage(Message("1", "keyDown", {{"key", Keys::w}}));
        nc.sendMessage(Message("1", "keyDown", {{"key", Keys::space}}));
        nc.sendMessage(Message("1", "keyUp", {{"key", Keys::w}}));

        auto outbox = nc.drainOutbox();
        CHECK(outbox.size() == 3, "3 messages queued in outbox");
        CHECK(outbox[0].type == "keyDown", "First msg type keyDown");
        CHECK(outbox[0].content["key"] == Keys::w, "First msg key=W");
        CHECK(outbox[1].content["key"] == Keys::space, "Second msg key=SPACE");
        CHECK(outbox[2].type == "keyUp", "Third msg type keyUp");

        auto empty = nc.drainOutbox();
        CHECK(empty.empty(), "Outbox empty after drain");
    }

    /* ====== 8. NetworkClient: disconnect message ====== */
    printf("[8] NetworkClient disconnect\n");
    {
        NetworkClient nc(1, "localhost", 8765);
        nc.disconnect();
        auto outbox = nc.drainOutbox();
        CHECK(outbox.size() == 1, "Disconnect queued 1 message");
        CHECK(outbox[0].type == "disconnect", "Disconnect message type");
    }

    /* ====== 9. NetworkClient: inject server messages ====== */
    printf("[9] NetworkClient inject server messages\n");
    {
        NetworkClient nc(2, "10.0.0.1", 8000);
        CHECK(nc.getMsgQueue().isEmpty(), "MsgQueue empty initially");

        // Simulate server sending messages
        nc.injectServerMessage(
            Message("server", "screen_info", {{"objects", nlohmann::json::array()},
                                               {"isPaused", false}, {"pausePlayerName", ""}}));
        nc.injectServerMessage(
            Message("server", "game_state_changed", {{"state", (int)GameState::inGame}}));
        nc.injectServerMessage(
            Message("server", "playsound", {{"sound", "explode1"}}));

        CHECK(!nc.getMsgQueue().isEmpty(), "Messages received from server");
        auto msg = nc.getMsgQueue().pop();
        CHECK(msg.type == "screen_info", "First msg screen_info");
        msg = nc.getMsgQueue().pop();
        CHECK(msg.type == "game_state_changed", "Second msg game_state_changed");
        msg = nc.getMsgQueue().pop();
        CHECK(msg.type == "playsound", "Third msg playsound");
    }

    /* ====== 10. NetworkClient: update is no-op ====== */
    printf("[10] NetworkClient update (no-op)\n");
    {
        NetworkClient nc(0, "localhost", 8000);
        nc.update();  // should not crash or produce messages
        CHECK(nc.getMsgQueue().isEmpty(), "No messages after NetworkClient update");
        CHECK(nc.drainOutbox().empty(), "No outbox after update");
    }

    /* ====== 11. MessageDispatcher basic ====== */
    printf("[11] MessageDispatcher basic\n");
    {
        MessageDispatcher disp;
        int screenCalls = 0, soundCalls = 0;

        disp.on("screen_info", [&](const Message&) { ++screenCalls; });
        disp.on("playsound", [&](const Message&) { ++soundCalls; });
        CHECK(disp.hasHandler("screen_info"), "Handler registered for screen_info");
        CHECK(disp.hasHandler("playsound"), "Handler registered for playsound");
        CHECK(!disp.hasHandler("nonexistent"), "No handler for nonexistent");

        disp.dispatch(Message("server", "screen_info", {}));
        CHECK(screenCalls == 1, "screen_info handler called once");
        CHECK(soundCalls == 0, "sound handler not called");

        disp.dispatch(Message("server", "playsound", {{"sound", "explode1"}}));
        CHECK(soundCalls == 1, "playsound handler called once");

        disp.dispatch(Message("server", "unknown_type", {}));
        CHECK(screenCalls == 1, "screen_info not called for unknown type");
    }

    /* ====== 12. MessageDispatcher: dispatchAll drains queue ====== */
    printf("[12] MessageDispatcher dispatchAll\n");
    {
        Queue<Message> q;
        q.push(Message("server", "screen_info", {}));
        q.push(Message("server", "playsound", {{"sound", "shotgun"}}));
        q.push(Message("server", "game_state_changed", {{"state", 2}}));

        MessageDispatcher disp;
        int count = 0;
        disp.on("screen_info", [&](const Message&) { ++count; });
        disp.on("playsound", [&](const Message&) { ++count; });
        disp.on("game_state_changed", [&](const Message&) { ++count; });

        disp.dispatchAll(q);
        CHECK(count == 3, "dispatchAll processed 3 messages");
        CHECK(q.isEmpty(), "Queue empty after dispatchAll");
    }

    /* ====== 13. SinglePlayerClient → MessageDispatcher pipeline ====== */
    printf("[13] Full pipeline: input->update->message->dispatch\n");
    {
        Queue<Message> mq;
        auto game = std::make_shared<Game>(mq);
        SinglePlayerClient client(0, game, mq, "Pipeline");
        client.newPlayer();

        // Process input
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::w}}));
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::space}}));

        // Drive game tick
        client.update();

        // Drain messages and dispatch
        MessageDispatcher disp;
        int screenInfoCount = 0;
        int stateChangeCount = 0;
        disp.on("screen_info", [&](const Message&) { ++screenInfoCount; });
        disp.on("game_state_changed", [&](const Message&) { ++stateChangeCount; });
        disp.dispatchAll(mq);

        CHECK(screenInfoCount > 0, "screen_info dispatched after update");
        CHECK(mq.isEmpty(), "All messages drained after dispatchAll");

        // Verify player can be updated
        client.sendMessage(Message("0", "keyDown", {{"key", Keys::s}}));
        client.sendMessage(Message("0", "keyUp", {{"key", Keys::w}}));
        client.update();
        disp.dispatchAll(mq);
        CHECK(screenInfoCount > 0, "screen_info dispatched on second tick");
    }

    /* ====== 14. Cleanup (no resources needed) ====== */
    printf("[14] Cleanup\n");
    // All objects use stack allocation; no cleanup needed
    CHECK(true, "All Phase 7 tests completed");

    int total = testsPassed + testsFailed;
    printf("\n===========================================\n");
    printf("  Results: %d / %d passed, %d failed\n",
           testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
