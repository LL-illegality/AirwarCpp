#include "Level.h"
#include "Constants.h"
#include "RNG.h"

Enemy EnemyBuilder::build() const {
    Enemy e;
    if (maxVelocity > 0) e.maxVelocity = maxVelocity;
    e.velocityMultiplier = velocityMultiplier;
    e.health = health;
    e.image = imageFromName(image);
    e.velocity = velocity;
    e.targetPos = targetPos;
    e.acceleration = acceleration;
    if (boundingBox) e.boundingBox = std::make_shared<BoundingBox>(*boundingBox);
    for (auto& item : inventory)
        e.inventory.push_back(static_cast<ItemTypes>(item));
    // TODO: weapons are not constructed here; Phase 2 will add weapons via factory
    return e;
}

Flag Level::splitFlagStatic(const Flag& flag, int avgLevel, int nukeBonus) {
    int total = (int)flag.unitTypeList.size();
    if (total <= 1) return flag;
    double ratio = (double)avgLevel / MIN_WEAPON_LEVEL_THRESHOLD;
    int frontCount = std::max(1, (int)(total * ratio + MIN_ENEMY_COUNT_THRESHOLD + avgLevel - MIN_WEAPON_LEVEL_THRESHOLD / 2) + nukeBonus);
    if (frontCount >= total) return flag;

    std::vector<int> indices(total);
    for (int i = 0; i < total; ++i) indices[i] = i;

    std::shuffle(indices.begin(), indices.end(), globalRNG());

    std::vector<int> pick(indices.begin(), indices.begin() + frontCount);
    std::unordered_set<int> pickSet(pick.begin(), pick.end());

    std::vector<std::string> frontUnits, backUnits;
    for (int i = 0; i < total; ++i) {
        if (pickSet.count(i)) frontUnits.push_back(flag.unitTypeList[i]);
        else backUnits.push_back(flag.unitTypeList[i]);
    }
    Flag frontFlag(frontUnits, flag.timeBeforeNext, flag.finishCondition);
    Flag backFlag(backUnits, 20, FlagFinishCondition::killAll);

    for (auto& item : flag.drops) {
        if (std::uniform_int_distribution<int>(0, 1)(globalRNG()) == 0)
            frontFlag.drops.push_back(item);
        else
            backFlag.drops.push_back(item);
    }
    return frontFlag;
}

Flag Level::splitFlag(const Flag& flag, int avgLevel, int nukeBonus) {
    int total = (int)flag.unitTypeList.size();
    if (total <= 1) return flag;
    double ratio = (double)avgLevel / MIN_WEAPON_LEVEL_THRESHOLD;
    int frontCount = std::max(1, (int)(total * ratio + MIN_ENEMY_COUNT_THRESHOLD + avgLevel - MIN_WEAPON_LEVEL_THRESHOLD / 2) + nukeBonus);
    if (frontCount >= total) return flag;

    std::vector<int> indices(total);
    for (int i = 0; i < total; ++i) indices[i] = i;

    std::shuffle(indices.begin(), indices.end(), globalRNG());

    std::vector<int> pick(indices.begin(), indices.begin() + frontCount);
    std::unordered_set<int> pickSet(pick.begin(), pick.end());

    std::vector<std::string> frontUnits, backUnits;
    for (int i = 0; i < total; ++i) {
        if (pickSet.count(i))
            frontUnits.push_back(flag.unitTypeList[i]);
        else
            backUnits.push_back(flag.unitTypeList[i]);
    }
    Flag frontFlag(frontUnits, flag.timeBeforeNext, flag.finishCondition);
    Flag backFlag(backUnits, 20, FlagFinishCondition::killAll);
    for (auto& item : flag.drops) {
        if (std::uniform_int_distribution<int>(0, 1)(globalRNG()) == 0)
            frontFlag.drops.push_back(item);
        else
            backFlag.drops.push_back(item);
    }
    flags.insert(flags.begin() + currFlagIndex + 1, std::move(backFlag));
    ++totalFlags;
    return frontFlag;
}
