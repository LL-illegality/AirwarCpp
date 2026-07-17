#pragma once
#include <variant>
#include <string>
#include "NetTypes.h"
#include "MsgTypes.h"
#include "BinarySerializer.h"
#include "../Core/Vector.h"

// NetMessage: variant-based universal message for client↔server
struct NetMessage {
    std::string sender;

    // Client messages
    using ClientVariant = std::variant<
        ConnectMsg, DisconnectMsg, GetMsg,
        KeyMsg, JoyAxisMsg, JoyHatMsg
    >;

    // Server messages
    using ServerVariant = std::variant<
        ConnectResponseMsg, ScreenInfoMsg, GameStateChangedMsg,
        PlaySoundMsg, ParticleEffectMsg, LoadLevelMsg, SetTitleMsg
    >;

    bool isClientMsg = false;
    ClientVariant clientMsg;
    ServerVariant serverMsg;

    NetMessage() = default;

    // Client-side constructors
    explicit NetMessage(const ConnectMsg& m) : isClientMsg(true), clientMsg(m), clientTypeVal(0) {}
    explicit NetMessage(const DisconnectMsg& m) : isClientMsg(true), clientMsg(m), clientTypeVal(1) {}
    explicit NetMessage(const GetMsg& m) : isClientMsg(true), clientMsg(m), clientTypeVal(2) {}
    explicit NetMessage(const KeyMsg& m, bool isUp = false)
        : isClientMsg(true), clientMsg(m), clientTypeVal(isUp ? 4 : 3) {}
    explicit NetMessage(const JoyAxisMsg& m) : isClientMsg(true), clientMsg(m), clientTypeVal(5) {}
    explicit NetMessage(const JoyHatMsg& m) : isClientMsg(true), clientMsg(m), clientTypeVal(6) {}

    // Server-side constructors
    explicit NetMessage(const ConnectResponseMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(0) {}
    explicit NetMessage(const ScreenInfoMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(1) {}
    explicit NetMessage(const GameStateChangedMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(2) {}
    explicit NetMessage(const PlaySoundMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(3) {}
    explicit NetMessage(const ParticleEffectMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(4) {}
    explicit NetMessage(const LoadLevelMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(5) {}
    explicit NetMessage(const SetTitleMsg& m) : isClientMsg(false), serverMsg(m), serverTypeVal(6) {}

    // ── JSON serialization ──
    nlohmann::json to_json() const {
        nlohmann::json content;
        ClientMsgType ctype;
        ServerMsgType stype;

        if (isClientMsg) {
            std::visit([&](auto& msg) {
                content = msg.to_json();
            }, clientMsg);
            return {{"sender", sender},
                    {"type", clientTypeName(getClientType())},
                    {"content", content}};
        } else {
            std::visit([&](auto& msg) {
                content = msg.to_json();
            }, serverMsg);
            return {{"sender", sender},
                    {"type", serverTypeName(getServerType())},
                    {"content", content}};
        }
    }

    std::string str() const { return to_json().dump(); }

    static NetMessage from_json(const std::string& jsonStr) {
        auto j = nlohmann::json::parse(jsonStr);
        NetMessage nm;
        nm.sender = j.value("sender", "");
        std::string type = j["type"].get<std::string>();
        auto& content = j["content"];

        // Try client types first
        auto ct = clientTypeFromName(type);
        if (ct != ClientMsgType::connect) {  // sentinel: not found uses connect
            // Check if it actually matched
        }

        if (type == "connect" && nm.sender != "server")
                                              { nm.clientMsg = ConnectMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 0; }
        else if (type == "connect")          { nm.serverMsg = ConnectResponseMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 0; }
        else if (type == "disconnect")      { nm.clientMsg = DisconnectMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 1; }
        else if (type == "get")             { nm.clientMsg = GetMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 2; }
        else if (type == "keyDown")         { nm.clientMsg = KeyMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 3; }
        else if (type == "keyUp")           { nm.clientMsg = KeyMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 4; }
        else if (type == "joyAxis")         { nm.clientMsg = JoyAxisMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 5; }
        else if (type == "joyHat")          { nm.clientMsg = JoyHatMsg::from_json(content); nm.isClientMsg = true; nm.clientTypeVal = 6; }
        else if (type == "screen_info")     { nm.serverMsg = ScreenInfoMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 1; }
        else if (type == "game_state_changed") { nm.serverMsg = GameStateChangedMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 2; }
        else if (type == "playsound")       { nm.serverMsg = PlaySoundMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 3; }
        else if (type == "particle_effect") { nm.serverMsg = ParticleEffectMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 4; }
        else if (type == "load_level")      { nm.serverMsg = LoadLevelMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 5; }
        else if (type == "set_title")       { nm.serverMsg = SetTitleMsg::from_json(content); nm.isClientMsg = false; nm.serverTypeVal = 6; }

        return nm;
    }

    // ── Type name helpers ──
    static const char* clientTypeName(ClientMsgType t) {
        switch (t) {
            case ClientMsgType::connect:    return "connect";
            case ClientMsgType::disconnect: return "disconnect";
            case ClientMsgType::get:        return "get";
            case ClientMsgType::keyDown:    return "keyDown";
            case ClientMsgType::keyUp:      return "keyUp";
            case ClientMsgType::joyAxis:    return "joyAxis";
            case ClientMsgType::joyHat:     return "joyHat";
        }
        return "unknown";
    }

    static const char* serverTypeName(ServerMsgType t) {
        switch (t) {
            case ServerMsgType::connect:            return "connect";
            case ServerMsgType::screen_info:        return "screen_info";
            case ServerMsgType::game_state_changed: return "game_state_changed";
            case ServerMsgType::playsound:          return "playsound";
            case ServerMsgType::particle_effect:    return "particle_effect";
            case ServerMsgType::load_level:         return "load_level";
            case ServerMsgType::set_title:          return "set_title";
        }
        return "unknown";
    }

    static ClientMsgType clientTypeFromName(const std::string& name) {
        if (name == "connect") return ClientMsgType::connect;
        if (name == "disconnect") return ClientMsgType::disconnect;
        if (name == "get") return ClientMsgType::get;
        if (name == "keyDown") return ClientMsgType::keyDown;
        if (name == "keyUp") return ClientMsgType::keyUp;
        if (name == "joyAxis") return ClientMsgType::joyAxis;
        if (name == "joyHat") return ClientMsgType::joyHat;
        return ClientMsgType::connect;  // fallback
    }

    static ServerMsgType serverTypeFromName(const std::string& name) {
        if (name == "connect") return ServerMsgType::connect;
        if (name == "screen_info") return ServerMsgType::screen_info;
        if (name == "game_state_changed") return ServerMsgType::game_state_changed;
        if (name == "playsound") return ServerMsgType::playsound;
        if (name == "particle_effect") return ServerMsgType::particle_effect;
        if (name == "load_level") return ServerMsgType::load_level;
        if (name == "set_title") return ServerMsgType::set_title;
        return ServerMsgType::connect;
    }

    // ── Type extraction helpers ──
    // We store the type explicitly because the variant index doesn't
    // match the protocol enum (keyDown/keyUp share KeyMsg variant).
    uint8_t clientTypeVal = 0;  // ClientMsgType value (only valid when isClientMsg)
    uint8_t serverTypeVal = 0;  // ServerMsgType value (only valid when !isClientMsg)

    ClientMsgType getClientType() const { return (ClientMsgType)clientTypeVal; }
    ServerMsgType getServerType() const { return (ServerMsgType)serverTypeVal; }
};

// ── Binary codec for NetMessage ──
struct NetMessageBinaryCodec {
    static std::vector<uint8_t> encode(const NetMessage& msg) {
        BinaryWriter w;
        if (msg.isClientMsg) {
            w.writeU8(0);  // client flag
            w.writeU8((uint8_t)msg.getClientType());
            std::visit([&](auto& m) {
                encodeClient(w, m);
            }, msg.clientMsg);
        } else {
            w.writeU8(1);  // server flag
            w.writeU8((uint8_t)msg.getServerType());
            std::visit([&](auto& m) {
                encodeServer(w, m);
            }, msg.serverMsg);
        }
        return w.data();
    }

    static NetMessage decode(const std::vector<uint8_t>& data) {
        BinaryReader r(data);
        uint8_t flag = r.readU8();
        uint8_t type = r.readU8();
        NetMessage msg;

        if (flag == 0) {
            switch ((ClientMsgType)type) {
                case ClientMsgType::connect: {
                    ConnectMsg m; m.playerName = r.readString(); return NetMessage(m);
                }
                case ClientMsgType::disconnect: return NetMessage(DisconnectMsg{});
                case ClientMsgType::get: return NetMessage(GetMsg{});
                case ClientMsgType::keyDown: return NetMessage(KeyMsg{r.readU16()});
                case ClientMsgType::keyUp: return NetMessage(KeyMsg{r.readU16()});
                case ClientMsgType::joyAxis: return NetMessage(JoyAxisMsg{r.readU8(), r.readFloat()});
                case ClientMsgType::joyHat: {
                    JoyHatMsg m; m.value = {(int8_t)r.readU8(), (int8_t)r.readU8()}; return NetMessage(m);
                }
            }
        } else {
            switch ((ServerMsgType)type) {
                case ServerMsgType::connect:
                    return NetMessage(ConnectResponseMsg{r.readU16()});
                default:
                    break;
            }
        }
        return msg;
    }

private:
    static void encodeClient(BinaryWriter& w, const ConnectMsg& m) { w.writeString(m.playerName); }
    static void encodeClient(BinaryWriter& w, const DisconnectMsg&) {}
    static void encodeClient(BinaryWriter& w, const GetMsg&) {}
    static void encodeClient(BinaryWriter& w, const KeyMsg& m) { w.writeU16((uint16_t)m.key); }
    static void encodeClient(BinaryWriter& w, const JoyAxisMsg& m) { w.writeU8((uint8_t)m.axis); w.writeFloat(m.value); }
    static void encodeClient(BinaryWriter& w, const JoyHatMsg& m) {
        w.writeU8((uint8_t)((int8_t)(m.value.size() > 0 ? m.value[0] : 0)));
        w.writeU8((uint8_t)((int8_t)(m.value.size() > 1 ? m.value[1] : 0)));
    }

    static void encodeServer(BinaryWriter& w, const ConnectResponseMsg& m) { w.writeU16((uint16_t)m.player_id); }
    static void encodeServer(BinaryWriter& w, const ScreenInfoMsg& m) {
        w.writeU16((uint16_t)m.objects.size());
        for (auto& o : m.objects) {
            w.writeU16((uint16_t)o.id);
            w.writeFloat(o.x); w.writeFloat(o.y); w.writeFloat(o.rotation);
            w.writeString(o.image);
        }
        w.writeBool(m.isPaused);
    }
    static void encodeServer(BinaryWriter& w, const GameStateChangedMsg& m) { w.writeU8((uint8_t)m.state); }
    static void encodeServer(BinaryWriter& w, const PlaySoundMsg& m) { w.writeString(m.sound); }
    static void encodeServer(BinaryWriter& w, const ParticleEffectMsg& m) { w.writeString(m.effect); w.writeFloat(m.x); w.writeFloat(m.y); }
    static void encodeServer(BinaryWriter& w, const LoadLevelMsg& m) { w.writeString(m.level); }
    static void encodeServer(BinaryWriter& w, const SetTitleMsg& m) { w.writeString(m.title); w.writeU16((uint16_t)m.duration); }
};
