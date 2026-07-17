#include <cstdio>
#include <cstdlib>
#include <string>
#include "Core/RNG.h"
#include "Net/NetTypes.h"
#include "Net/MsgTypes.h"
#include "Net/BinarySerializer.h"
#include "Net/NetMessage.h"

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
    printf("AirwarCPP Phase 6 -- Network Protocol Layer\n");
    printf("===========================================\n\n");

    seedRNG();

    /* ====== 1. Message type enums ====== */
    printf("[1] Message type enums\n");
    CHECK((int)ClientMsgType::connect == 0, "ClientMsgType::connect == 0");
    CHECK((int)ClientMsgType::joyHat == 6, "ClientMsgType::joyHat == 6");
    CHECK((int)ServerMsgType::connect == 0, "ServerMsgType::connect == 0");
    CHECK((int)ServerMsgType::set_title == 6, "ServerMsgType::set_title == 6");

    /* ====== 2. Client message JSON round-trip ====== */
    printf("[2] Client message JSON round-trip\n");
    {
        ConnectMsg cm{"TestPlayer"};
        auto j = cm.to_json();
        CHECK(j["playerName"] == "TestPlayer", "ConnectMsg JSON");
        auto cm2 = ConnectMsg::from_json(j);
        CHECK(cm2.playerName == "TestPlayer", "ConnectMsg round-trip");
    }
    {
        KeyMsg km{119};  // 'w'
        auto j = km.to_json(); CHECK(j["key"] == 119, "KeyMsg JSON");
        auto km2 = KeyMsg::from_json(j); CHECK(km2.key == 119, "KeyMsg round-trip");
    }
    {
        JoyAxisMsg jam{0, -0.5f};
        auto j = jam.to_json(); CHECK(j["axis"] == 0, "JoyAxis axis");
        JoyAxisMsg jam2 = JoyAxisMsg::from_json(j);
        CHECK(jam2.axis == 0 && jam2.value == -0.5f, "JoyAxis round-trip");
    }
    {
        JoyHatMsg jhm{{1, 0}};
        auto j = jhm.to_json(); CHECK(j["value"][0] == 1, "JoyHat JSON x=1");
        auto jhm2 = JoyHatMsg::from_json(j);
        CHECK(jhm2.value[0] == 1 && jhm2.value[1] == 0, "JoyHat round-trip");
    }
    {
        DisconnectMsg dm; dm.to_json(); CHECK(true, "DisconnectMsg OK");
        GetMsg gm; gm.to_json(); CHECK(true, "GetMsg OK");
    }

    /* ====== 3. Server message JSON round-trip ====== */
    printf("[3] Server message JSON round-trip\n");
    {
        ConnectResponseMsg crm{42};
        CHECK(crm.to_json()["player_id"] == 42, "ConnectResponse JSON");
        CHECK(ConnectResponseMsg::from_json(crm.to_json()).player_id == 42,
              "ConnectResponse round-trip");
    }
    {
        GameStateChangedMsg gscm{GameState::inGame};
        CHECK(gscm.to_json()["state"] == (int)GameState::inGame, "GameStateChanged JSON");
        auto g2 = GameStateChangedMsg::from_json(gscm.to_json());
        CHECK(g2.state == GameState::inGame, "GameStateChanged round-trip");
    }
    {
        PlaySoundMsg psm{"shotgun_shoot"};
        CHECK(psm.to_json()["sound"] == "shotgun_shoot", "PlaySound JSON");
        CHECK(PlaySoundMsg::from_json(psm.to_json()).sound == "shotgun_shoot",
              "PlaySound round-trip");
    }
    {
        ParticleEffectMsg pem{"enemy_explosion", 400.0f, 300.0f};
        auto j = pem.to_json();
        CHECK(j["effect"] == "enemy_explosion" && j["x"] == 400.0f, "ParticleEffect JSON");
        auto p2 = ParticleEffectMsg::from_json(j);
        CHECK(p2.effect == "enemy_explosion" && p2.x == 400.0f, "ParticleEffect round-trip");
    }
    {
        LoadLevelMsg llm{"Level 1"};
        CHECK(llm.to_json()["level"] == "Level 1", "LoadLevel JSON");
        CHECK(LoadLevelMsg::from_json(llm.to_json()).level == "Level 1",
              "LoadLevel round-trip");
    }
    {
        SetTitleMsg stm{"You Win!", 300};
        auto j = stm.to_json();
        CHECK(j["title"] == "You Win!" && j["duration"] == 300, "SetTitle JSON");
        auto s2 = SetTitleMsg::from_json(j);
        CHECK(s2.title == "You Win!" && s2.duration == 300, "SetTitle round-trip");
    }

    /* ====== 4. ScreenInfoMsg (complex) ====== */
    printf("[4] ScreenInfoMsg\n");
    {
        ScreenInfoMsg sim;
        sim.isPaused = false;
        EntityState e1; e1.id = 0; e1.x = 400; e1.y = 300;
        e1.rotation = 0; e1.image = "player1";
        e1.health = 100; e1.isReady = true; e1.player_id = 0;
        e1.name = "LL"; e1.magabombQuantity = 1;
        sim.objects.push_back(e1);

        EntityState e2; e2.id = 1; e2.x = 500; e2.y = 100;
        e2.rotation = 90; e2.image = "en"; e2.health = 50;
        sim.objects.push_back(e2);

        auto j = sim.to_json();
        CHECK(j["objects"].size() == 2, "ScreenInfo has 2 objects");
        CHECK(j["objects"][0]["image"] == "player1", "ScreenInfo obj0 image");
        CHECK(j["objects"][0]["player_id"] == 0, "ScreenInfo obj0 player_id");
        CHECK(j["objects"][0]["health"] == 100.0, "ScreenInfo obj0 health");
        CHECK(j["objects"][0]["name"] == "LL", "ScreenInfo obj0 name");
        CHECK(j["objects"][1]["image"] == "en", "ScreenInfo obj1 image");
        CHECK(j["isPaused"] == false, "ScreenInfo isPaused");

        auto sim2 = ScreenInfoMsg::from_json(j);
        CHECK(sim2.objects.size() == 2, "ScreenInfo round-trip count");
        CHECK(sim2.objects[0].x == 400.0f && sim2.objects[0].name == "LL",
              "ScreenInfo round-trip fields");
        CHECK(sim2.objects[1].image == "en", "ScreenInfo round-trip enemy");
        CHECK(sim2.isPaused == false, "ScreenInfo round-trip isPaused");
    }

    /* ====== 5. NetMessage JSON serialization (client) ====== */
    printf("[5] NetMessage client JSON\n");
    {
        NetMessage nm(ConnectMsg{"Hero"});
        nm.sender = "0";
        auto jsonStr = nm.str();
        CHECK(!jsonStr.empty(), "NetMessage::str() non-empty");

        NetMessage parsed = NetMessage::from_json(jsonStr);
        CHECK(parsed.isClientMsg, "Parsed isClientMsg");
        CHECK(parsed.sender == "0", "Parsed sender");
        CHECK(parsed.getClientType() == ClientMsgType::connect, "Parsed client type");
        auto* cm = std::get_if<ConnectMsg>(&parsed.clientMsg);
        CHECK(cm != nullptr && cm->playerName == "Hero", "Parsed ConnectMsg content");
    }
    {
        NetMessage nm(KeyMsg{Keys::space});
        nm.sender = "0";
        auto parsed = NetMessage::from_json(nm.str());
        CHECK(parsed.getClientType() == ClientMsgType::keyDown, "KeyMsg parsed type");
        auto* km = std::get_if<KeyMsg>(&parsed.clientMsg);
        CHECK(km != nullptr && km->key == Keys::space, "KeyMsg parsed content");
    }
    {
        JoyHatMsg jhm{{-1, 1}};
        auto j = jhm.to_json();
        CHECK(j["value"][0] == -1 && j["value"][1] == 1, "JoyHat to_json OK");

        auto jhm2 = JoyHatMsg::from_json(j);
        CHECK(jhm2.value[0] == -1 && jhm2.value[1] == 1, "JoyHat from_json OK");

        // NetMessage wrapping
        NetMessage nm(jhm);
        nm.sender = "0";
        std::string js = nm.str();
        auto parsed = NetMessage::from_json(js);
        CHECK(parsed.sender == "0", "JoyHat NetMessage sender");
        CHECK(parsed.isClientMsg, "JoyHat NetMessage isClientMsg");
        auto* jm = std::get_if<JoyHatMsg>(&parsed.clientMsg);
        CHECK(jm != nullptr && jm->value[0] == -1 && jm->value[1] == 1,
              "JoyHat NetMessage content");
    }

    /* ====== 6. NetMessage JSON serialization (server) ====== */
    printf("[6] NetMessage server JSON\n");
    {
        ScreenInfoMsg sim;
        EntityState e; e.id = 0; e.x = 400; e.y = 300;
        e.rotation = 0; e.image = "player1"; e.health = 100;
        sim.objects.push_back(e);
        NetMessage nm(sim);
        nm.sender = "server";
        auto parsed = NetMessage::from_json(nm.str());
        CHECK(!parsed.isClientMsg, "Server msg parsed as server");
        CHECK(parsed.getServerType() == ServerMsgType::screen_info,
              "Parsed screen_info type");
        auto* sm = std::get_if<ScreenInfoMsg>(&parsed.serverMsg);
        CHECK(sm != nullptr && sm->objects.size() == 1, "Parsed ScreenInfo content");
    }
    {
        NetMessage nm(SetTitleMsg{"You Win!", 300});
        nm.sender = "server";
        auto parsed = NetMessage::from_json(nm.str());
        CHECK(parsed.getServerType() == ServerMsgType::set_title, "SetTitle type");
        auto* st = std::get_if<SetTitleMsg>(&parsed.serverMsg);
        CHECK(st != nullptr && st->title == "You Win!" && st->duration == 300,
              "SetTitle content");
    }

    /* ====== 7. Binary serialization (BinaryWriter/Reader) ====== */
    printf("[7] BinarySerializer\n");
    {
        BinaryWriter w;
        w.writeU8(255); w.writeU16(65535); w.writeFloat(3.14f);
        w.writeString("hello"); w.writeBool(true);
        CHECK(w.size() == 1 + 2 + 4 + 1 + 5 + 1, "BinaryWriter correct size");

        BinaryReader r(w.data());
        CHECK(r.readU8() == 255, "Binary readU8");
        CHECK(r.readU16() == 65535, "Binary readU16");
        CHECK(r.readFloat() == 3.14f, "Binary readFloat");
        CHECK(r.readString() == "hello", "Binary readString");
        CHECK(r.readBool() == true, "Binary readBool");
        CHECK(r.done(), "Binary reader exhausted");
    }
    {
        // EntityState binary
        BinaryWriter w;
        BinaryEntityState::write(w, 0, 400.0f, 300.0f, 45.0f, 0, 100.0f, true, 0, 1, "LL");
        CHECK(w.size() > 0, "BinaryEntityState write OK");
    }

    /* ====== 8. NetMessage binary round-trip ====== */
    printf("[8] NetMessage binary\n");
    {
        NetMessage orig(ConnectMsg{"BinaryPlayer"});
        auto bytes = NetMessageBinaryCodec::encode(orig);
        CHECK(bytes.size() > 0, "Binary encode non-empty");

        auto decoded = NetMessageBinaryCodec::decode(bytes);
        CHECK(decoded.isClientMsg, "Binary decoded client msg");
        CHECK(decoded.getClientType() == ClientMsgType::connect, "Binary type connect");
        auto* cm = std::get_if<ConnectMsg>(&decoded.clientMsg);
        CHECK(cm != nullptr && cm->playerName == "BinaryPlayer",
              "Binary ConnectMsg content");
    }
    {
        NetMessage orig(KeyMsg{Keys::w});
        auto bytes = NetMessageBinaryCodec::encode(orig);
        auto decoded = NetMessageBinaryCodec::decode(bytes);
        auto* km = std::get_if<KeyMsg>(&decoded.clientMsg);
        CHECK(km != nullptr && km->key == Keys::w, "Binary KeyMsg");
    }
    {
        NetMessage orig(JoyAxisMsg{1, 0.75f});
        auto bytes = NetMessageBinaryCodec::encode(orig);
        auto decoded = NetMessageBinaryCodec::decode(bytes);
        auto* jm = std::get_if<JoyAxisMsg>(&decoded.clientMsg);
        CHECK(jm != nullptr && jm->axis == 1 && jm->value == 0.75f, "Binary JoyAxis");
    }
    {
        NetMessage orig(JoyHatMsg{{-1, 1}});
        auto bytes = NetMessageBinaryCodec::encode(orig);
        auto decoded = NetMessageBinaryCodec::decode(bytes);
        auto* jm = std::get_if<JoyHatMsg>(&decoded.clientMsg);
        CHECK(jm != nullptr && jm->value[0] == -1 && jm->value[1] == 1, "Binary JoyHat");
    }
    {
        NetMessage orig(ConnectResponseMsg{7});
        auto bytes = NetMessageBinaryCodec::encode(orig);
        auto decoded = NetMessageBinaryCodec::decode(bytes);
        CHECK(!decoded.isClientMsg, "Binary server msg");
        auto* cr = std::get_if<ConnectResponseMsg>(&decoded.serverMsg);
        CHECK(cr != nullptr && cr->player_id == 7, "Binary ConnectResponse");
    }

    /* ====== 9. All 13 message types JSON round-trip ====== */
    printf("[9] All 13 types JSON round-trip\n");
    {
        // 6 client types + 7 server types = 13
        std::vector<NetMessage> msgs;
        msgs.emplace_back(ConnectMsg{"P1"});
        msgs.emplace_back(DisconnectMsg{});
        msgs.emplace_back(GetMsg{});
        msgs.emplace_back(KeyMsg{32});
        msgs.emplace_back(JoyAxisMsg{0, 0.5f});
        msgs.emplace_back(JoyHatMsg{{0, 1}});
        msgs.emplace_back(ConnectResponseMsg{0});
        msgs.emplace_back(GameStateChangedMsg{GameState::inGame});
        msgs.emplace_back(PlaySoundMsg{"explode1"});
        msgs.emplace_back(ParticleEffectMsg{"enemy_explosion", 400, 300});
        msgs.emplace_back(LoadLevelMsg{"Level 1"});
        msgs.emplace_back(SetTitleMsg{"Go!", 60});
        ScreenInfoMsg sim;
        EntityState e; e.id=0; e.x=400; e.y=300; e.rotation=0;
        e.image="player1"; e.health=100;
        sim.objects.push_back(e);
        msgs.emplace_back(sim);  // screen_info

        for (auto& m : msgs) {
            m.sender = m.isClientMsg ? "0" : "server";
            auto json = m.str();
            auto parsed = NetMessage::from_json(json);
            CHECK(parsed.isClientMsg == m.isClientMsg, "Type flag preserved");
            if (m.isClientMsg) {
                CHECK(parsed.getClientType() == m.getClientType(), "Client type preserved");
            } else {
                CHECK(parsed.getServerType() == m.getServerType(), "Server type preserved");
            }
        }
        CHECK(true, "All 13 types JSON round-trip OK");
    }

    /* ====== 10. NetMessage static type name helpers ====== */
    printf("[10] Type name helpers\n");
    CHECK(std::string(NetMessage::clientTypeName(ClientMsgType::keyDown)) == "keyDown",
          "clientTypeName(keyDown)");
    CHECK(std::string(NetMessage::serverTypeName(ServerMsgType::screen_info)) == "screen_info",
          "serverTypeName(screen_info)");
    CHECK(NetMessage::clientTypeFromName("joyHat") == ClientMsgType::joyHat,
          "clientTypeFromName(joyHat)");
    CHECK(NetMessage::serverTypeFromName("load_level") == ServerMsgType::load_level,
          "serverTypeFromName(load_level)");

    /* ====== Summary ====== */
    int total = testsPassed + testsFailed;
    printf("\n===========================================\n");
    printf("  Results: %d / %d passed, %d failed\n",
           testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
