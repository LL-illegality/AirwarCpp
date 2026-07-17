#pragma once
#include "../Core/Entity.h"
#include "../Core/Constants.h"
#include <random>

struct Item : Entity {
    ItemTypes itemType = ItemTypes::shotgun;
    int lifetime = 20 * GAME_TICK;
    int redirectionDelay = 0;
    int alpha = 255;
    int typeCycleTimer = 0;

    static std::unordered_map<ItemTypes, ItemTypes> cycleMap;
    static std::mt19937 rng;

    explicit Item(ItemTypes type);
    void update() override;
    void onCollision(Entity& other) override;
};
