#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__DoubleChestContainer_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__DoubleChestContainer_H__

#include "ChestTileEntity.h"
#include "../../../item/ItemInstance.h"
#include "../../../entity/player/Player.h"

// Wraps two ChestTileEntity halves as a single 54-slot container.
// Created transiently when opening a double chest; never added to the level.
class DoubleChestContainer : public ChestTileEntity {
    static const int HALF = 27;
public:
    DoubleChestContainer(ChestTileEntity* a, ChestTileEntity* b)
    :   ChestTileEntity()
    ,   chestA(a), chestB(b)
    {
        numTotalSlots = HALF * 2;
        items.resize(HALF * 2, NULL);

        for (int i = 0; i < HALF; i++) {
            ItemInstance* src = a->getItem(i);
            items[i] = (src && !src->isNull()) ? new ItemInstance(*src) : NULL;
        }
        for (int i = 0; i < HALF; i++) {
            ItemInstance* src = b->getItem(i);
            items[HALF + i] = (src && !src->isNull()) ? new ItemInstance(*src) : NULL;
        }

        clientSideOnly = true;
        runningId = a->runningId;
        hasCheckedNeighbors = true;
    }

    ~DoubleChestContainer() {
        // Write items back to the real chests before FillingContainer frees them
        for (int i = 0; i < HALF; i++)
            chestA->setItem(i, items[i]);
        for (int i = 0; i < HALF; i++)
            chestB->setItem(i, items[HALF + i]);
    }

    int getContainerSize() const { return HALF * 2; }
    std::string getName() const { return "Large Chest"; }
    bool shouldSave() { return false; }
    void clearCache() {}
    void tick() {}
    bool stillValid(Player* player) {
        return chestA->stillValid(player) && chestB->stillValid(player);
    }

private:
    ChestTileEntity* chestA;
    ChestTileEntity* chestB;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__DoubleChestContainer_H__*/
