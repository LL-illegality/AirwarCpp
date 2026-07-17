#pragma once
#include <uv.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include "UdpSocket.h"
#include "ReliableChannel.h"
#include "ConnectionManager.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "../Game/Game.h"

// ── UDP Game Server ────────────────────────────────────────────
class UdpServer {
    uv_loop_t* loop_ = nullptr;
    UdpSocket socket_;
    uv_timer_t gameTick_{};
    uv_timer_t cleanupTick_{};
    ConnectionManager connMgr_;
    std::shared_ptr<Game> game_;
    Queue<Message>* msgQueue_ = nullptr;
    int port_ = 8000;
    bool running_ = false;

    // Per-client reliable channels
    struct ClientSession {
        std::unique_ptr<ReliableChannel> channel;
        sockaddr_storage addr;
    };
    std::unordered_map<uint32_t, ClientSession> sessions_;

public:
    UdpServer() { gameTick_.data = this; cleanupTick_.data = this; }

    void setGame(std::shared_ptr<Game> g) { game_ = std::move(g); msgQueue_ = &game_->msgQueue; }

    bool start(int port) {
        port_ = port;
        loop_ = new uv_loop_t;
        uv_loop_init(loop_);

        if (!socket_.init(loop_)) return false;
        if (!socket_.bind(port_)) return false;
        if (!socket_.startRecv([this](const std::string& data, const sockaddr_storage& addr) {
            onRecv(data, addr);
        })) return false;

        uv_timer_init(loop_, &gameTick_);
        uv_timer_start(&gameTick_, [](uv_timer_t* t) {
            ((UdpServer*)t->data)->onGameTick();
        }, 33, 33);

        uv_timer_init(loop_, &cleanupTick_);
        uv_timer_start(&cleanupTick_, [](uv_timer_t* t) {
            ((UdpServer*)t->data)->onCleanupTick();
        }, 100, 100);

        running_ = true;
        return true;
    }

    void run() {
        if (loop_ && running_) uv_run(loop_, UV_RUN_DEFAULT);
    }

    void runAsync() {
        if (loop_ && running_) uv_run(loop_, UV_RUN_NOWAIT);
    }

    void stop() {
        running_ = false;
        uv_timer_stop(&gameTick_);
        uv_timer_stop(&cleanupTick_);
        socket_.close();
        if (loop_) { uv_loop_close(loop_); delete loop_; loop_ = nullptr; }
    }

    bool isRunning() const { return running_; }
    ConnectionManager& connections() { return connMgr_; }

private:
    void onRecv(const std::string& data, const sockaddr_storage& addr) {
        // Parse header (6 bytes minimum)
        if (data.size() < 6) return;
        auto hdr = PacketHeader::decode((const uint8_t*)data.data());

        // Find or create session
        auto* conn = connMgr_.findByAddr(addr);
        uint32_t cid;
        if (!conn) {
            cid = connMgr_.createConnection(addr);
            conn = connMgr_.get(cid);
            if (!conn) return;
            auto ch = std::make_unique<ReliableChannel>(
                [this, addr](const std::vector<uint8_t>& pkt) {
                    socket_.sendTo((const char*)pkt.data(), pkt.size(),
                                   (const struct sockaddr*)&addr);
                });
            sessions_[cid] = {std::move(ch), addr};
        } else {
            cid = conn->clientId;
        }
        connMgr_.updateRecvTime(cid);

        auto& session = sessions_[cid];
        auto payload = session.channel->receive((const uint8_t*)data.data(), data.size());

        // Route message based on type
        if (payload.size() >= 1) {
            uint8_t msgType = payload[0];
            std::string content((const char*)payload.data() + 1, payload.size() - 1);
            handleMessage(cid, msgType, content);
        }
    }

    void handleMessage(uint32_t cid, uint8_t msgType, const std::string& content) {
        if (!game_) return;
        auto* conn = connMgr_.get(cid);
        if (!conn) return;

        // msgType: 0=connect, 1=disconnect, 2=get, 3=keyDown, 4=keyUp, 5=joyAxis, 6=joyHat
        if (msgType == 0) {  // connect
            if (game_->currState() == GameState::mainMenu) {
                int pid = 0;
                while (game_->board.findPlayer(pid)) ++pid;
                auto player = std::make_shared<Player>(pid);
                player->x = 400; player->y = 2.0/3.0 * 600;
                player->image = (pid % 2 == 1) ? Images::player2 : Images::player1;
                conn->playerId = pid;
                game_->board.addPlayer(player);
                // Send connect response
                Message resp("server", "connect", {{"player_id", pid}});
                sendToClient(cid, resp);
            } else {
                Message resp("server", "connect", {{"player_id", -1}});
                sendToClient(cid, resp);
            }
        }
        else if (msgType == 1) {  // disconnect
            auto dp = game_->board.findPlayer(conn->playerId);
            if (dp) { dp->isAlive = false; }
            connMgr_.remove(cid);
            sessions_.erase(cid);
        }
        else if (msgType == 2) {  // get
            broadcastGameState();
        }
        else if (msgType == 3 || msgType == 4) {  // keyDown/Up
            int key = content.empty() ? 0 : (int)(uint8_t)content[0];
            auto p = game_->board.findPlayer(conn->playerId);
            if (p) {
                if (msgType == 3) {
                    if (key == Keys::p) {
                        if (!game_->isPaused) {
                            game_->isPaused = true;
                            game_->pausePlayerId = p->player_id;
                            game_->pausePlayerName = p->name;
                        } else if (game_->pausePlayerId == p->player_id) {
                            game_->isPaused = false;
                            game_->pausePlayerId = -1;
                            game_->pausePlayerName = "";
                        }
                        game_->getObjects();
                    } else {
                        p->pressedKeyList.push_back(key);
                    }
                } else {
                    auto& kl = p->pressedKeyList;
                    auto it = std::find(kl.begin(), kl.end(), key);
                    if (it != kl.end()) kl.erase(it);
                }
            }
        }
        else if (msgType == 5) {  // joyAxis
            if (content.size() >= 5) {
                int axis = (uint8_t)content[0];
                float value;
                memcpy(&value, content.data() + 1, 4);
                auto p = game_->board.findPlayer(conn->playerId);
                if (p) {
                    if ((int)p->joystickAxisList.size() <= axis)
                        p->joystickAxisList.resize(axis + 1);
                    p->joystickAxisList[axis] = (std::abs(value) < 0.2f) ? 0 : value;
                }
            }
        }
        else if (msgType == 6) {  // joyHat
            if (content.size() >= 2) {
                int x = (int8_t)content[0];
                int y = (int8_t)content[1];
                auto p = game_->board.findPlayer(conn->playerId);
                if (p) {
                    auto& kl = p->pressedKeyList;
                    auto rm = [&](int k) { kl.erase(std::remove(kl.begin(), kl.end(), k), kl.end()); };
                    rm(Keys::w); rm(Keys::s); rm(Keys::a); rm(Keys::d);
                    if (x == 1) kl.push_back(Keys::d);
                    if (x == -1) kl.push_back(Keys::a);
                    if (y == 1) kl.push_back(Keys::w);
                    if (y == -1) kl.push_back(Keys::s);
                }
            }
        }
    }

    void onGameTick() {
        if (!game_) return;
        if (!game_->isPaused) {
            game_->update();
        } else {
            game_->getObjects();
        }
        game_->detectLevelState();
        broadcastGameState();
    }

    void onCleanupTick() {
        for (auto& [cid, ch] : sessions_) ch.channel->update();
        connMgr_.checkTimeouts(20000);
    }

    void broadcastGameState() {
        if (!msgQueue_ || sessions_.empty()) {
            if (msgQueue_) msgQueue_->clear();
            return;
        }
        Message msg;
        while (msgQueue_->tryPop(msg)) {
            auto bytes = serializeMessage(msg);
            for (auto& [cid, session] : sessions_) {
                socket_.sendTo((const char*)bytes.data(), bytes.size(),
                               (const struct sockaddr*)&session.addr);
            }
        }
    }

    void sendToClient(uint32_t cid, const Message& msg) {
        auto it = sessions_.find(cid);
        if (it == sessions_.end()) return;
        auto bytes = serializeMessage(msg);
        socket_.sendTo((const char*)bytes.data(), bytes.size(),
                       (const struct sockaddr*)&it->second.addr);
    }

    std::vector<uint8_t> serializeMessage(const Message& msg) {
        std::string json = msg.str();
        std::vector<uint8_t> data;
        data.push_back((uint8_t)json.size());
        data.insert(data.end(), json.begin(), json.end());
        return data;
    }
};
