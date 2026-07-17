#pragma once
#include "../Core/Entity.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "Item.h"
#include <memory>
#include <vector>

class Board {
public:
    Queue<Message>* msgQueue = nullptr;

    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<Entity>> units;  // holds Unit and Item subclasses
    std::vector<std::shared_ptr<Projectile>> projectiles;

    Board() = default;
    explicit Board(Queue<Message>& mq) : msgQueue(&mq) {}

    int currId = -1;
    void increaseId() { ++currId; }

    std::shared_ptr<Player> nearestPlayer(double x, double y) const;
    std::shared_ptr<Unit> nearestUnit(double x, double y, Race race) const;
    std::shared_ptr<Unit> nearestEnemyInCone(double x, double y, double coneAngle,
                                              const Vector& forwardDir, Race targetRace) const;
    std::shared_ptr<Player> findPlayer(int player_id) const;

    void addPlayer(std::shared_ptr<Player> player);
    void addUnit(std::shared_ptr<Entity> entity, const std::string& type);
    bool isAllPlayerPrepared() const;

    nlohmann::json getScreenObjects() const;
    void update();
    void checkCollision(std::shared_ptr<Entity> item, SpatialGrid& grid);

    Message generateSoundMessage(const std::string& sound) const;
    Message generateParticleMessage(const std::string& effect, double x, double y) const;

    std::vector<std::shared_ptr<Entity>> getAllObjects() const;
};
