#pragma once
#include "../Core/Message.h"
#include <unordered_map>
#include <functional>
#include <memory>

class MessageDispatcher {
    using Handler = std::function<void(const Message&)>;
    std::unordered_map<std::string, Handler> handlers_;

public:
    void on(const std::string& type, Handler handler) {
        handlers_[type] = std::move(handler);
    }

    void off(const std::string& type) {
        handlers_.erase(type);
    }

    bool dispatch(const Message& msg) const {
        auto it = handlers_.find(msg.type);
        if (it != handlers_.end()) {
            it->second(msg);
            return true;
        }
        return false;
    }

    void dispatchAll(Queue<Message>& queue) {
        Message msg;
        while (queue.tryPop(msg)) {
            dispatch(msg);
        }
    }

    bool hasHandler(const std::string& type) const {
        return handlers_.find(type) != handlers_.end();
    }

    void clear() { handlers_.clear(); }
};
