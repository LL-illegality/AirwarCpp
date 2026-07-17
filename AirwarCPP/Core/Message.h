#pragma once
#include <string>
#include <unordered_map>
#include "json.hpp"

struct Message {
    std::string sender;
    std::string type;
    nlohmann::json content;

    Message() = default;
    Message(std::string sender_, std::string type_, nlohmann::json content_)
        : sender(std::move(sender_)), type(std::move(type_)), content(std::move(content_)) {}

    nlohmann::json to_json() const {
        return {{"sender", sender}, {"type", type}, {"content", content}};
    }

    std::string str() const {
        return to_json().dump();
    }

    static Message from_json(const nlohmann::json& j) {
        return {j["sender"].get<std::string>(),
                j["type"].get<std::string>(),
                j["content"]};
    }

    static Message from_string(const std::string& s) {
        return from_json(nlohmann::json::parse(s));
    }
};
