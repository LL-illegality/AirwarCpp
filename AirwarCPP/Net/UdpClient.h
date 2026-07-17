#pragma once
#include <uv.h>
#include <string>
#include "UdpSocket.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"

class UdpClient {
    uv_loop_t loop_{};
    bool loopInit_ = false;
    UdpSocket socket_;
    sockaddr_storage serverAddr_{};
    bool connected_ = false;
    int playerId_ = -1;
    Queue<Message> incoming_;
    uint64_t lastPongMs_ = 0;
    int pingIntervalMs_ = 3000;
    int reconnectAttempts_ = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 3;
    std::string savedIp_;
    int savedPort_ = 0;
    std::string savedName_;

public:
    UdpClient() = default;
    ~UdpClient() { disconnect(); }

    Queue<Message>& incoming() { return incoming_; }
    int playerId() const { return playerId_; }
    bool isConnected() const { return connected_; }
    uint64_t msSinceLastPong() const {
        return lastPongMs_ == 0 ? 0 : (nowMs() - lastPongMs_);
    }

    bool connect(const std::string& ip, int port, const std::string& playerName) {
        savedIp_ = ip; savedPort_ = port; savedName_ = playerName;
        return doConnect(playerName, -1);
    }

    // Reconnect with saved player_id
    bool reconnect() {
        if (savedIp_.empty() || ++reconnectAttempts_ > MAX_RECONNECT_ATTEMPTS) return false;
        if (!doConnect(savedName_, playerId_)) return false;
        // Send reconnect message
        std::vector<uint8_t> payload = {(uint8_t)8, (uint8_t)(playerId_ >= 0 ? playerId_ : 0)};
        std::vector<uint8_t> pkt; pkt.push_back((uint8_t)payload.size());
        pkt.insert(pkt.end(), payload.begin(), payload.end());
        socket_.sendTo((const char*)pkt.data(), pkt.size(), (const struct sockaddr*)&serverAddr_);
        uv_run(&loop_, UV_RUN_NOWAIT);
        return true;
    }

    void sendMessage(const Message& msg) {
        if (!connected_) return;
        sendNow(msg);
    }

    void tick() {
        if (!connected_) return;
        uv_run(&loop_, UV_RUN_NOWAIT);
    }

    void disconnect() {
        if (connected_) {
            sendNow(Message(std::to_string(playerId_ >= 0 ? playerId_ : 0), "disconnect", {}));
            connected_ = false;
        }
        socket_.close();
        if (loopInit_) { uv_loop_close(&loop_); loopInit_ = false; }
    }

    void processIncoming(std::function<void(const Message&)> handler = nullptr) {
        Message m;
        while (incoming_.tryPop(m)) {
            if (handler) handler(m);
        }
    }

private:
    bool doConnect(const std::string& name, int existingPid) {
        if (loopInit_) { uv_loop_close(&loop_); uv_loop_init(&loop_); }
        else { uv_loop_init(&loop_); loopInit_ = true; }
        if (!socket_.init(&loop_)) return false;
        if (!UdpSocket::makeAddr(savedIp_, savedPort_, serverAddr_)) return false;

        struct sockaddr_in any;
        uv_ip4_addr("0.0.0.0", 0, &any);
        uv_udp_bind(socket_.handle(), (const struct sockaddr*)&any, 0);

        socket_.startRecv([this](const std::string& data, const sockaddr_storage&) {
            size_t offset = 0;
            while (offset < data.size()) {
                uint8_t len = (uint8_t)data[offset];
                if (offset + 1 + len > data.size()) break;
                std::string inner = data.substr(offset + 1, len);
                offset += 1 + len;

                // Check for binary ping (msgType=7, single byte payload)
                if (inner.size() == 1 && (uint8_t)inner[0] == 7) {
                    // Send pong back (msgType=7, empty)
                    std::vector<uint8_t> pkt = {1, 7};
                    socket_.sendTo((const char*)pkt.data(), pkt.size(), (const struct sockaddr*)&serverAddr_);
                    uv_run(&loop_, UV_RUN_NOWAIT);
                    continue;
                }

                // Normal JSON message
                try {
                    auto msg = Message::from_string(inner);
                    if (msg.type == "connect" && msg.content.contains("player_id")) {
                        playerId_ = msg.content["player_id"].get<int>();
                        lastPongMs_ = nowMs();
                    }
                    incoming_.push(msg);
                } catch (...) {}
            }
        });

        connected_ = true;
        lastPongMs_ = nowMs();
        reconnectAttempts_ = 0;

        // Send connect
        sendNow(Message("0", "connect", {{"playerName", name}}));
        return true;
    }

    void sendNow(const Message& msg) {
        std::string json = msg.str();
        std::vector<uint8_t> data;
        data.push_back((uint8_t)json.size());
        data.insert(data.end(), json.begin(), json.end());
        socket_.sendTo((const char*)data.data(), data.size(), (const struct sockaddr*)&serverAddr_);
        uv_run(&loop_, UV_RUN_NOWAIT);
    }

    static uint64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
