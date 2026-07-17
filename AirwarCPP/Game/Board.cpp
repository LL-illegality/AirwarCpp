#include "Board.h"
#include "../Core/Constants.h"
#include "../Core/RNG.h"
#include <algorithm>
#include <cmath>
#include <random>



std::shared_ptr<Player> Board::nearestPlayer(double x, double y) const {
    if (players.empty()) return nullptr;
    auto it = std::min_element(players.begin(), players.end(),
        [x, y](auto& a, auto& b) {
            return std::pow(a->x - x, 2) + std::pow(a->y - y, 2) <
                   std::pow(b->x - x, 2) + std::pow(b->y - y, 2);
        });
    return *it;
}

std::shared_ptr<Unit> Board::nearestUnit(double x, double y, Race race) const {
    std::vector<std::shared_ptr<Unit>> candidates;
    for (auto& u : units) {
        auto unit = std::dynamic_pointer_cast<Unit>(u);
        if (unit && unit->race == race) candidates.push_back(unit);
    }
    if (candidates.empty()) return nullptr;
    auto it = std::min_element(candidates.begin(), candidates.end(),
        [x, y](auto& a, auto& b) {
            return std::pow(a->x - x, 2) + std::pow(a->y - y, 2) <
                   std::pow(b->x - x, 2) + std::pow(b->y - y, 2);
        });
    return *it;
}

std::shared_ptr<Unit> Board::nearestEnemyInCone(double x, double y, double coneAngle,
                                                  const Vector& forwardDir, Race targetRace) const {
    std::vector<std::shared_ptr<Unit>> candidates;
    for (auto& u : units) {
        auto unit = std::dynamic_pointer_cast<Unit>(u);
        if (unit && unit->race == targetRace) candidates.push_back(unit);
    }
    double forwardAngle = ~forwardDir;
    double halfAngle = coneAngle / 2.0;
    std::shared_ptr<Unit> best = nullptr;
    double bestDist = 0;
    for (auto& enemy : candidates) {
        Vector toEnemy(enemy->x - x, enemy->y - y);
        double dist = toEnemy.length();
        if (dist == 0) continue;
        double enemyAngle = ~toEnemy;
        double diff = std::abs(enemyAngle - forwardAngle);
        diff = std::fmod(diff, 360.0);
        if (diff > 180) diff = 360 - diff;
        if (diff <= halfAngle) {
            if (!best || dist < bestDist) {
                best = enemy;
                bestDist = dist;
            }
        }
    }
    return best;
}

std::shared_ptr<Player> Board::findPlayer(int player_id) const {
    for (auto& p : players)
        if (p->player_id == player_id) return p;
    return nullptr;
}

void Board::addPlayer(std::shared_ptr<Player> player) {
    players.push_back(player);
    increaseId();
    player->id = currId;
}

void Board::addUnit(std::shared_ptr<Entity> entity, const std::string& type) {
    if (type == "unit") {
        if (!std::dynamic_pointer_cast<Item>(entity))
            entity->x = std::uniform_int_distribution<int>(0, SCREEN_W)(globalRNG());
        units.push_back(entity);
        increaseId();
        entity->id = currId;
    } else if (type == "projectile") {
        auto asProj = std::dynamic_pointer_cast<Projectile>(entity);
        if (asProj) {
            projectiles.push_back(asProj);
            increaseId();
            asProj->id = currId;
        }
    }
}

bool Board::isAllPlayerPrepared() const {
    return !players.empty() && std::all_of(players.begin(), players.end(),
        [](auto& p) { return p->isReady; });
}

nlohmann::json Board::getScreenObjects() const {
    nlohmann::json arr = nlohmann::json::array();
    for (auto& obj : getAllObjects()) {
        if (!obj->image.has_value()) continue;
        nlohmann::json entry = {
            {"id", obj->id}, {"x", obj->x}, {"y", obj->y},
            {"rotation", obj->rotation},
            {"image", ImageNames.at(obj->image.value())}
        };
        if (auto p = std::dynamic_pointer_cast<Player>(obj)) {
            entry["health"] = p->health;
            entry["isReady"] = p->isReady;
            entry["player_id"] = p->player_id;
            entry["name"] = p->name;
            entry["magabombQuantity"] = p->magabombQuantity;
        }
        if (auto item = std::dynamic_pointer_cast<Item>(obj))
            entry["alpha"] = item->alpha;
        arr.push_back(entry);
    }
    return arr;
}

std::vector<std::shared_ptr<Entity>> Board::getAllObjects() const {
    std::vector<std::shared_ptr<Entity>> all;
    for (auto& p : players) all.push_back(p);
    for (auto& u : units) all.push_back(u);
    for (auto& p : projectiles) all.push_back(p);
    return all;
}

Message Board::generateSoundMessage(const std::string& sound) const {
    return Message("server", "playsound", {{"sound", sound}});
}

Message Board::generateParticleMessage(const std::string& effect, double x, double y) const {
    return Message("server", "particle_effect", {{"effect", effect}, {"x", x}, {"y", y}});
}

static std::mt19937& boardRng() { return globalRNG(); }

void Board::checkCollision(std::shared_ptr<Entity> item, SpatialGrid& grid) {
    for (auto& other : grid.getNearby(item.get())) {
        if (item.get() != other && (*item & *other)) {
            bool wasAlive = item->isAlive;
            item->onCollision(*other);
            if (wasAlive && !item->isAlive) {
                if (std::dynamic_pointer_cast<Missile>(item))
                    msgQueue->push(generateParticleMessage("missile_hit", item->x, item->y));
                else if (std::dynamic_pointer_cast<Rocket>(item) ||
                         std::dynamic_pointer_cast<RocketEnemy>(item))
                    msgQueue->push(generateParticleMessage("rocket_hit", item->x, item->y));
                else if (std::dynamic_pointer_cast<Bullet>(item))
                    msgQueue->push(generateParticleMessage("bullet_hit", item->x, item->y));
                else if (std::dynamic_pointer_cast<Lazer>(item))
                    msgQueue->push(generateParticleMessage("lazer_hit", item->x, item->y));
                else if (std::dynamic_pointer_cast<AutocannonShells>(item))
                    msgQueue->push(generateParticleMessage("autocannon_hit", item->x, item->y));
            }
        }
    }
}

void Board::update() {
    SpatialGrid grid;
    auto allObj = getAllObjects();
    for (auto& obj : allObj) grid.add(obj.get());

    for (auto& obj : allObj) {
        obj->update();

        if (obj->x < -100 || obj->x > SCREEN_W + 100 ||
            obj->y < -100 || obj->y > SCREEN_H + 100) {
            obj->isAlive = false;
        }

        auto mg = std::dynamic_pointer_cast<Magabomb>(obj);
        if (mg && mg->isExploding) {
            for (auto it = units.begin(); it != units.end(); ) {
                auto unit = std::dynamic_pointer_cast<Unit>(*it);
                if (unit) {
                    for (auto& invItem : unit->inventory) {
                        auto drop = std::make_shared<Item>(invItem);
                        drop->x = (*it)->x;
                        drop->y = (*it)->y;
                        addUnit(drop, "unit");
                    }
                }
                msgQueue->push(generateSoundMessage("explode" + std::to_string(std::uniform_int_distribution<int>(1,5)(boardRng()))));
                it = units.erase(it);
            }
            for (auto it = projectiles.begin(); it != projectiles.end(); )
                it = projectiles.erase(it);
            msgQueue->push(generateSoundMessage("nuclear_missile_explode"));
            msgQueue->push(generateParticleMessage("nuke_explosion", mg->x, mg->y));
            continue;
        }

        auto unit = std::dynamic_pointer_cast<Unit>(obj);
        if (unit && unit->weapon && unit->weapon->isShooting()) {
            auto bullets = unit->weapon->shoot(unit->x, unit->y);
            for (auto& b : bullets) {
                if (!b) continue;
                if (b->chooseTargetAngle < 360) {
                    auto target = (b->shooterRace == Race::player)
                        ? nearestEnemyInCone(b->x, b->y, b->chooseTargetAngle, Vector(0, -1), Race::enemy)
                        : nearestEnemyInCone(b->x, b->y, b->chooseTargetAngle, Vector(0, 1), Race::player);
                    if (target) {
                        b->faceToTarget(*target, false);
                        b->faceToTarget(*target, true);
                    }
                }
                if (b->chooseTarget) {
                    if (b->shooterRace == Race::enemy) {
                        auto target = nearestPlayer(b->x, b->y);
                        if (target) {
                            b->faceToTarget(*target, false);
                            b->faceToTarget(*target, true);
                            b->chooseTarget = false;
                        }
                    } else {
                        b->chooseTarget = false;
                    }
                }
                if (std::dynamic_pointer_cast<Missile>(b) || std::dynamic_pointer_cast<Rocket>(b)) {
                    if (b->shooterRace == Race::enemy)
                        std::dynamic_pointer_cast<Missile>(b)->target = nearestPlayer(b->x, b->y);
                    else
                        std::dynamic_pointer_cast<Missile>(b)->target = nearestUnit(b->x, b->y, Race::enemy);
                }
                addUnit(b, "projectile");
            }
        }

        auto player = std::dynamic_pointer_cast<Player>(obj);
        if (player && player->isThrowingMagabomb) {
            player->isThrowingMagabomb = false;
            auto mb = std::make_shared<Magabomb>();
            mb->x = player->x;
            mb->y = player->y;
            addUnit(mb, "projectile");
            msgQueue->push(generateSoundMessage("nuclear_missile_shoot"));
        }

        checkCollision(obj, grid);

        auto u = std::dynamic_pointer_cast<Unit>(obj);
        if (u && u->health <= 0) {
            u->isAlive = false;
            msgQueue->push(generateParticleMessage(
                std::dynamic_pointer_cast<Player>(u) ? "player_explosion" : "enemy_explosion",
                u->x, u->y));
        }

        if (!obj->isAlive) {
            auto deadUnit = std::dynamic_pointer_cast<Unit>(obj);
            if (deadUnit) {
                for (auto& invType : deadUnit->inventory) {
                    auto drop = std::make_shared<Item>(invType);
                    drop->x = deadUnit->x;
                    drop->y = deadUnit->y;
                    addUnit(drop, "unit");
                }
            }
            auto deadPlayer = std::dynamic_pointer_cast<Player>(obj);
            if (deadPlayer) {
                for (auto& gType : deadPlayer->gottenItem) {
                    auto drop = std::make_shared<Item>(gType);
                    drop->x = deadPlayer->x;
                    drop->y = deadPlayer->y;
                    addUnit(drop, "unit");
                }
                deadPlayer->gottenItem.clear();
            }
            if (std::dynamic_pointer_cast<Unit>(obj)) {
                msgQueue->push(generateSoundMessage("explode" + std::to_string(std::uniform_int_distribution<int>(1,5)(boardRng()))));
            }
        }
    }

    players.erase(std::remove_if(players.begin(), players.end(),
        [](auto& p) { return !p->isAlive; }), players.end());
    units.erase(std::remove_if(units.begin(), units.end(),
        [](auto& u) { return !u->isAlive; }), units.end());
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
        [](auto& p) { return !p->isAlive; }), projectiles.end());
}
