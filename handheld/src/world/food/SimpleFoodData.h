#ifndef NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__
#define NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__

//package net.minecraft.world.food;

class FoodItem;
class Player;

class SimpleFoodData
{
public:
    SimpleFoodData();

    void eat(int food);
    void eat(FoodItem* item);

    void tick(Player* player);
    
    // New methods for hunger/health system
    void addHunger(int amount, float saturation);
    void sprintConsume();  // Called when sprinting
    void setHunger(int hunger);
    void setSaturation(float sat) { saturation = sat < 0.0f ? 0.0f : (sat > 20.0f ? 20.0f : sat); }
    float getSaturation() const { return saturation; }
    int getHunger() const { return foodLevel; }
    
private:
    int foodLevel;
    float saturation;
    int hungerTickTimer = 0;
    int starvationTimer = 0;
    float lastFoodTickTime = 0.0f;
    float walkHungerSeconds = 0.0f;
    float sprintHungerSeconds = 0.0f;
};

#endif /*NET_MINECRAFT_WORLD_FOOD__SimpleFoodData_H__*/
