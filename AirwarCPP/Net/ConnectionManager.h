#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <uv.h>

enum class ConnectionState : uint8_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

struct Connection {
    uint32_t clientId = 0;
    int playerId = -1;
    sockaddr_storage addr;
    ConnectionState state = ConnectionState::DISCONNECTED;
    uint64_t lastRecvTime = 0;
    uint64_t createdTime = 0;

    bool isTimedOut(uint64_t now, uint64_t timeoutMs = 20000) const {
        return now - lastRecvTime > timeoutMs;
    }
};

class ConnectionManager {
    uint32_t nextClientId_ = 1;
    std::unordered_map<uint32_t, Connection> connections_;
    std::function<bool(uint32_t)> timeoutCallback_;

public:
    void setTimeoutCallback(std::function<bool(uint32_t)> cb) {
        timeoutCallback_ = std::move(cb);
    }

    uint32_t createConnection(const sockaddr_storage& addr) {
        uint32_t id = nextClientId_++;
        uint64_t now = nowMs();
        auto& conn = connections_[id];
        conn.clientId = id;
        conn.addr = addr;
        conn.state = ConnectionState::CONNECTING;
        conn.lastRecvTime = now;
        conn.createdTime = now;
        return id;
    }

    Connection* get(uint32_t clientId) {
        auto it = connections_.find(clientId);
        return it != connections_.end() ? &it->second : nullptr;
    }

    Connection* findByAddr(const sockaddr_storage& addr) {
        for (auto& [id, conn] : connections_)
            if (memcmp(&conn.addr, &addr, sizeof(addr)) == 0)
                return &conn;
        return nullptr;
    }

    void setConnected(uint32_t clientId) {
        auto* conn = get(clientId);
        if (conn) conn->state = ConnectionState::CONNECTED;
    }

    void remove(uint32_t clientId) {
        connections_.erase(clientId);
    }

    void updateRecvTime(uint32_t clientId) {
        auto* conn = get(clientId);
        if (conn) conn->lastRecvTime = nowMs();
    }

    void checkTimeouts(uint64_t timeoutMs = 20000) {
        uint64_t now = nowMs();
        for (auto it = connections_.begin(); it != connections_.end(); ) {
            if (it->second.isTimedOut(now, timeoutMs)) {
                it->second.state = ConnectionState::DISCONNECTED;
                if (timeoutCallback_ && timeoutCallback_(it->first)) {
                    it = connections_.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    int count() const { return (int)connections_.size(); }
    int activeCount() const {
        int n = 0;
        for (auto& [id, c] : connections_)
            if (c.state == ConnectionState::CONNECTED) ++n;
        return n;
    }

    void clear() { connections_.clear(); nextClientId_ = 1; }

private:
    static uint64_t nowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};
