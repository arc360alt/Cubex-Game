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

    // Use actual horizontal displacement so dedicated/multiplayer players drain food too.
    const float dx = player->x - player->xo;
    const float dz = player->z - player->zo;
    const float horizontalDistance = Mth::sqrt(dx * dx + dz * dz);

    // Time-based hunger rates (independent of server tick speed):
    // - walking: 1 hunger bar (2 points) per 60s => 1 point per 30s
    // - sprinting: 1 hunger bar (2 points) per 25s => 1 point per 12.5s
    static const float WALK_SECONDS_PER_POINT = 30.0f;
    static const float SPRINT_SECONDS_PER_POINT = 12.5f;

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
    // 0.16f was far too low and treated regular movement like sprinting.
    // Use a much higher threshold so only true sprint-speed movement is counted.
    const bool isSprinting = speed > 2.2f;

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
}

void SimpleFoodData::setHunger(int hunger) {
    foodLevel = Mth::Min(Mth::Max(hunger, 0), 20);
    if (foodLevel > 0) {
        // Cancel any pending starvation so the player can't be damaged
        // immediately after food is restored (e.g. on respawn).
        starvationTimer = 0;
        walkHungerSeconds = 0.0f;
        sprintHungerSeconds = 0.0f;
        lastFoodTickTime = 0.0f;  // reset timing so first post-respawn tick starts fresh
    }
}

