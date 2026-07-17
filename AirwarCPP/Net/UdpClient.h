#pragma once
#include <uv.h>
#include <string>
#include "UdpSocket.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"

class UdpClient {
    uv_loop_t loop_{};
    UdpSocket socket_;
    sockaddr_storage serverAddr_{};
    bool connected_ = false;
    int playerId_ = -1;
    Queue<Message> incoming_;
    uv_timer_t pollTimer_{};

public:
    UdpClient() { pollTimer_.data = this; }
    ~UdpClient() { disconnect(); }

    Queue<Message>& incoming() { return incoming_; }
    int playerId() const { return playerId_; }
    bool isConnected() const { return connected_; }

    bool connect(const std::string& ip, int port, const std::string& playerName) {
        uv_loop_init(&loop_);
        if (!socket_.init(&loop_)) return false;
        if (!UdpSocket::makeAddr(ip, port, serverAddr_)) return false;

        struct sockaddr_in any;
        uv_ip4_addr("0.0.0.0", 0, &any);
        uv_udp_bind(socket_.handle(), (const struct sockaddr*)&any, 0);

        socket_.startRecv([this](const std::string& data, const sockaddr_storage&) {
            size_t offset = 0;
            while (offset < data.size()) {
                uint8_t len = (uint8_t)data[offset];
                if (offset + 1 + len > data.size()) break;
                std::string json = data.substr(offset + 1, len);
                offset += 1 + len;
                try {
                    auto msg = Message::from_string(json);
                    if (msg.type == "connect" && msg.content.contains("player_id"))
                        playerId_ = msg.content["player_id"].get<int>();
                    incoming_.push(msg);
                } catch (...) {}
            }
        });

        connected_ = true;

        // Send connect message
        sendNow(Message("0", "connect", {{"playerName", playerName}}));
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
        uv_loop_close(&loop_);
    }

    // Process incoming — call from main thread
    void processIncoming(std::function<void(const Message&)> handler = nullptr) {
        Message m;
        while (incoming_.tryPop(m)) {
            if (handler) handler(m);
        }
    }

private:
    void sendNow(const Message& msg) {
        std::string json = msg.str();
        std::vector<uint8_t> data;
        data.push_back((uint8_t)json.size());
        data.insert(data.end(), json.begin(), json.end());
        socket_.sendTo((const char*)data.data(), data.size(),
                       (const struct sockaddr*)&serverAddr_);
        uv_run(&loop_, UV_RUN_NOWAIT);
    }
};
