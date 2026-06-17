#include "SimpleFoodData.h"
#include "FoodConstants.h"
#include "../item/FoodItem.h"
#include "../entity/player/Player.h"
#include "../Difficulty.h"
#include "../../platform/time.h"

SimpleFoodData::SimpleFoodData()
:	foodLevel(20),  // Start with full hunger like Minecraft
	saturation(5)   // Start with some saturation too
{
}

void SimpleFoodData::eat( int food ) {
	foodLevel = Mth::Min(food + foodLevel, FoodConstants::MAX_FOOD);
}

void SimpleFoodData::eat( FoodItem* item ) {
	eat(item->getNutrition());
}

void SimpleFoodData::tick(Player* player) {


    // Skip hunger logic if in creative mode
    if (player->isCreativeModeAllowed()) {
        foodLevel = 20;
        saturation = 20.0f;
        return;
    }

    if (player->level->difficulty == Difficulty::PEACEFUL) {
        foodLevel = 20;
        saturation = 20.0f;
        return;
    }

    // Health regen when food is high enough
    // Saturation is not consumed here — it is already drained by sprinting/walking.
    if (foodLevel >= 18 && player->health < player->getMaxHealth()) {
        if (player->tickCount % 10 == 0) {
            player->heal(1);
        }
    }

    // Track position ourselves so this works on both server (where xo==x due to lerpTo)
    // and client. lastTickPosValid gates the first tick so we don't get a huge jump.
    float horizontalDistance = 0.0f;
    if (lastTickPosValid) {
        const float dx = player->x - lastTickX;
        const float dz = player->z - lastTickZ;
        horizontalDistance = Mth::sqrt(dx * dx + dz * dz);
    }
    lastTickX = player->x;
    lastTickZ = player->z;
    lastTickPosValid = true;

    // Time-based hunger rates (independent of server tick speed):
    // - walking: 1 hunger point per 90s (~1.5 min)
    // - sprinting: 1 hunger point per 45s
    static const float WALK_SECONDS_PER_POINT = 90.0f;
    static const float SPRINT_SECONDS_PER_POINT = 45.0f;

    const float now = getTimeS();
    if (lastFoodTickTime <= 0.0f) {
        lastFoodTickTime = now;
    }
    float dt = now - lastFoodTickTime;
    lastFoodTickTime = now;

    // Clamp bad deltas (pauses, clock jumps).
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 0.25f) dt = 0.25f;

    const bool isMoving = horizontalDistance > 0.001f;
    const float speed = (dt > 0.0f) ? (horizontalDistance / dt) : 0.0f;
    // Walk speed is ~4.3 blocks/s; sprint speed is ~5.6 blocks/s.
    // Threshold at 5.0 so normal walking is never misclassified as sprinting.
    const bool isSprinting = speed > 5.0f;

    if (isMoving) {
        if (isSprinting) {
            sprintHungerSeconds += dt;
            while (sprintHungerSeconds >= SPRINT_SECONDS_PER_POINT) {
                sprintHungerSeconds -= SPRINT_SECONDS_PER_POINT;
                if (foodLevel > 0) foodLevel--;
                if (saturation > 0.0f) {
                    saturation -= 0.5f;
                    if (saturation < 0.0f) saturation = 0.0f;
                }
            }
        } else {
            walkHungerSeconds += dt;
            while (walkHungerSeconds >= WALK_SECONDS_PER_POINT) {
                walkHungerSeconds -= WALK_SECONDS_PER_POINT;
                if (foodLevel > 0) foodLevel--;
            }
        }
    } else {
        hungerTickTimer = 0;
    }

    // Starvation damage
    if (foodLevel <= 0) {
        starvationTimer++;
        if (starvationTimer >= 80) {
            starvationTimer = 0;
            if (player->health > 1)
                player->hurt(player, 1);
        }
    } else {
        starvationTimer = 0;
    }
}

void SimpleFoodData::sprintConsume() {
    if (saturation > 0) {
        saturation -= 0.5f;  // Sprint costs saturation first
    } else if (foodLevel > 0) {
        foodLevel -= 1;  // Then hunger
    }
}

void SimpleFoodData::addHunger(int amount, float sat) {
    foodLevel = Mth::Min(foodLevel + amount, 20);
    saturation = Mth::Min(saturation + sat, 20.0f);
    starvationTimer = 0;
    walkHungerSeconds = 0.0f;
    sprintHungerSeconds = 0.0f;
    lastFoodTickTime = 0.0f;
    lastTickPosValid = false;
}

void SimpleFoodData::setHunger(int hunger) {
    foodLevel = Mth::Min(Mth::Max(hunger, 0), 20);
    if (foodLevel > 0) {
        // Cancel any pending starvation so the player can't be damaged
        // immediately after food is restored (e.g. on respawn).
        starvationTimer = 0;
        walkHungerSeconds = 0.0f;
        sprintHungerSeconds = 0.0f;
        lastFoodTickTime = 0.0f;
        lastTickPosValid = false;
    }
}

