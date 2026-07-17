#pragma once
#include "../Core/Constants.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "../Game/Board.h"
#include "../Game/Item.h"
#include <memory>
#include <functional>

// ── Abstract base class ────────────────────────────────────────
class GameClient {
public:
    virtual ~GameClient() = default;
    virtual void sendMessage(const Message& msg) = 0;
    virtual void update() = 0;
    virtual void disconnect() = 0;
    virtual Queue<Message>& getMsgQueue() = 0;
    virtual int getPlayerId() const = 0;
    virtual const std::string& getPlayerName() const = 0;
};

// ── SinglePlayerClient ─────────────────────────────────────────
class SinglePlayerClient : public GameClient {
    int playerId_ = 0;
    std::string playerName_ = "{default}";
    std::shared_ptr<Game> game_;
    Queue<Message>& msgQueue_;

public:
    SinglePlayerClient(int pid, std::shared_ptr<Game> g, Queue<Message>& mq,
                       const std::string& name = "{default}")
        : playerId_(pid), playerName_(name), game_(std::move(g)), msgQueue_(mq) {}

    int getPlayerId() const override { return playerId_; }
    const std::string& getPlayerName() const override { return playerName_; }
    Queue<Message>& getMsgQueue() override { return msgQueue_; }
    std::shared_ptr<Game> game() const { return game_; }

    void newPlayer() {
        if (!game_) return;
        auto player = std::make_shared<Player>(playerId_);
        player->x = SCREEN_W / 2.0;
        player->y = 2.0 / 3.0 * SCREEN_H;
        player->image = Images::player1;
        player->name = (playerName_ == "{default}") ? std::to_string(playerId_) : playerName_;
        game_->board.addPlayer(player);
    }

    void sendMessage(const Message& msg) override {
        if (!game_) return;
        auto player = game_->board.findPlayer(std::stoi(msg.sender));
        if (!player) return;

        if (msg.type == "keyDown") {
            int key = msg.content["key"].get<int>();
            if (key == Keys::p) {
                if (!game_->isPaused) {
                    game_->isPaused = true;
                    game_->pausePlayerId = player->player_id;
                    game_->pausePlayerName = player->name;
                } else if (game_->pausePlayerId == player->player_id) {
                    game_->isPaused = false;
                    game_->pausePlayerId = -1;
                    game_->pausePlayerName = "";
                }
                game_->getObjects();
            } else {
                player->pressedKeyList.push_back(key);
            }
        } else if (msg.type == "keyUp") {
            int key = msg.content["key"].get<int>();
            auto it = std::find(player->pressedKeyList.begin(), player->pressedKeyList.end(), key);
            if (it != player->pressedKeyList.end()) player->pressedKeyList.erase(it);
        } else if (msg.type == "joyAxis") {
            int axis = msg.content["axis"].get<int>();
            double value = msg.content["value"].get<double>();
            if ((int)player->joystickAxisList.size() <= axis)
                player->joystickAxisList.resize(axis + 1);
            player->joystickAxisList[axis] = (std::abs(value) < 0.2) ? 0 : value;
        } else if (msg.type == "joyHat") {
            auto val = msg.content["value"];
            auto& kl = player->pressedKeyList;
            auto rm = [&](int k) { kl.erase(std::remove(kl.begin(), kl.end(), k), kl.end()); };
            if (val[0] == 0 && val[1] == 0) { rm(Keys::w); rm(Keys::s); rm(Keys::a); rm(Keys::d); }
            else {
                if (val[0].get<int>() == 1) kl.push_back(Keys::d);
                if (val[0].get<int>() == -1) kl.push_back(Keys::a);
                if (val[1].get<int>() == 1) kl.push_back(Keys::w);
                if (val[1].get<int>() == -1) kl.push_back(Keys::s);
            }
        }
    }

    void update() override {
        if (game_) game_->update();
    }

    void disconnect() override {
        // Single-player: no-op
    }
};

// ── NetworkClient (Phase 9: UDP communication, requires UdpClient.h) ──
// Note: Include Net/UdpClient.h before this file if using NetworkClient
class NetworkClient : public GameClient {
    std::string playerName_ = "{default}";
    Queue<Message> msgQueue_;
    std::unique_ptr<UdpClient> udp_;
    bool connected_ = false;

public:
    NetworkClient(int, const std::string& ip, int port,
                  const std::string& name = "{default}")
        : playerName_(name) {
        udp_ = std::make_unique<UdpClient>();
        connected_ = udp_->connect(ip, port, playerName_);
    }

    ~NetworkClient() override { disconnect(); }

    int getPlayerId() const override { return udp_ ? udp_->playerId() : -1; }
    const std::string& getPlayerName() const override { return playerName_; }
    Queue<Message>& getMsgQueue() override { return msgQueue_; }
    bool isConnected() const { return connected_; }

    void sendMessage(const Message& msg) override {
        if (udp_) udp_->sendMessage(msg);
    }

    void update() override {
        if (udp_) {
            udp_->processIncoming([this](const Message& msg) {
                msgQueue_.push(msg);
            });
        }
    }

    void disconnect() override {
        if (udp_) { udp_->disconnect(); udp_.reset(); }
        connected_ = false;
    }

    // For testing: direct access
    UdpClient* udp() { return udp_.get(); }
};
