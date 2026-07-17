#include "Item.h"
#include "../Core/Constants.h"
#include "../Core/RNG.h"

std::unordered_map<ItemTypes, ItemTypes> Item::cycleMap = {
    {ItemTypes::shotgun, ItemTypes::lazer},
    {ItemTypes::lazer, ItemTypes::autocannon},
    {ItemTypes::autocannon, ItemTypes::shotgun},
    {ItemTypes::missile, ItemTypes::rocket},
    {ItemTypes::rocket, ItemTypes::missile}
};

std::mt19937 Item::rng{42};

Item::Item(ItemTypes type)
    : itemType(type), typeCycleTimer(std::uniform_int_distribution<int>(3, 5)(rng) * GAME_TICK) {
    auto it = ItemImageMap.find(type);
    if (it != ItemImageMap.end()) image = it->second;
    boundingBox = std::make_shared<BoundingBox>(24, 24);
    velocity = Vector(std::uniform_int_distribution<int>(-5, 5)(rng),
                      std::uniform_int_distribution<int>(-5, 5)(rng));
    velocityMultiplier = 1;
}

void Item::update() {
    Entity::update();
    --lifetime;
    if (lifetime <= 0) { isAlive = false; return; }

    --typeCycleTimer;
    if (typeCycleTimer <= 0) {
        typeCycleTimer = std::uniform_int_distribution<int>(3, 5)(rng) * GAME_TICK;
        auto it = cycleMap.find(itemType);
        if (it != cycleMap.end()) {
            itemType = it->second;
            auto img = ItemImageMap.find(itemType);
            if (img != ItemImageMap.end()) image = img->second;
        }
    }

    if (redirectionDelay >= 0) {
        if (x <= 0 || x >= SCREEN_W) { velocity.x *= -1; redirectionDelay = 15; }
        if (y <= 0 || y >= SCREEN_H) { velocity.y *= -1; redirectionDelay = 15; }
    } else {
        --redirectionDelay;
    }

    if (lifetime < 3 * GAME_TICK) {
        double progress = (double)lifetime / (3.0 * GAME_TICK);
        int blinkInterval = std::max(2, (int)(progress * progress * 20));
        alpha = ((lifetime / blinkInterval) % 2 == 0) ? 255 : 50;
    }
}

void Item::onCollision(Entity& other) {
    if (other.race == Race::player) {
        if (auto* p = dynamic_cast<Player*>(&other)) {
            if (p->isAlive) {
                p->gottenItem.push_back(itemType);
                isAlive = false;
            }
        }
    }
}
