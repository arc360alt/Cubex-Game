#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HayBaleFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HayBaleFeature_H__

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../../../util/Random.h"

class HayBaleFeature : public Feature
{
public:
    HayBaleFeature() : Feature(false) {}

    bool place(Level* level, Random* random, int x, int y, int z) override {
        int count = 2 + random->nextInt(5);  // 2-6 bales per clump
        int placed = 0;
        for (int i = 0; i < count; i++) {
            int ox = x + random->nextInt(7) - random->nextInt(7);
            int oz = z + random->nextInt(7) - random->nextInt(7);
            int oy = level->getHeightmap(ox, oz);
            int below = level->getTile(ox, oy - 1, oz);
            if (level->isEmptyTile(ox, oy, oz) &&
                (below == Tile::grass->id || below == Tile::dirt->id)) {
                level->setTileNoUpdate(ox, oy, oz, Tile::hayBale->id);
                placed++;
            }
        }
        return placed > 0;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__HayBaleFeature_H__*/
