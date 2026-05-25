#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"
#include "LeafTile.h"
#include "../../Facing.h"

class TreeTile: public Tile
{
public:
	static const int NORMAL_TRUNK = 0;
    static const int DARK_TRUNK   = 1;
    static const int BIRCH_TRUNK  = 2;

	// Axis bits stored in data bits 2-3
	static const int AXIS_Y = 0; // upright (default)
	static const int AXIS_X = 1; // east-west
	static const int AXIS_Z = 2; // north-south
	static const int AXIS_MASK = 0xC; // bits 2-3
	static const int TYPE_MASK = 0x3; // bits 0-1

	TreeTile(int id)
	:	Tile(id, Material::wood)
	{
        tex = 20;
    }

    int getResourceCount(Random* random) {
        return 1;
    }

    int getResource(int data, Random* random) {
        return Tile::treeTrunk->id;
    }

    void onRemove(Level* level, int x, int y, int z) {
        int r = LeafTile::REQUIRED_WOOD_RANGE;
        int r2 = r + 1;

        if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
            for (int xo = -r; xo <= r; xo++)
            for (int yo = -r; yo <= r; yo++)
            for (int zo = -r; zo <= r; zo++) {
                int t = level->getTile(x + xo, y + yo, z + zo);
                if (t == Tile::leaves->id) {
                    int currentData = level->getData(x + xo, y + yo, z + zo);
                    if ((currentData & LeafTile::UPDATE_LEAF_BIT) == 0) {
                        level->setDataNoUpdate(x + xo, y + yo, z + zo, currentData | LeafTile::UPDATE_LEAF_BIT);
                    }
                }
            }
        }
    }

    int getTexture(int face, int data) {
        int woodType = data & TYPE_MASK;
        int axis     = (data & AXIS_MASK) >> 2;

        // Which faces are the "ends" (show log-top texture) depends on axis
        bool isEnd;
        if (axis == AXIS_X)
            isEnd = (face == Facing::WEST || face == Facing::EAST);
        else if (axis == AXIS_Z)
            isEnd = (face == Facing::NORTH || face == Facing::SOUTH);
        else // AXIS_Y (default, upright)
            isEnd = (face == Facing::DOWN || face == Facing::UP);

        if (isEnd) return 21; // log top/end texture

        if (woodType == DARK_TRUNK)  return 4 + 7 * 16;
        if (woodType == BIRCH_TRUNK) return 5 + 7 * 16;
        return 20; // oak side
    }

    // Set log axis based on the face of the block it was placed against
    int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face,
                                 float clickX, float clickY, float clickZ, int itemValue) {
        int woodType = itemValue & TYPE_MASK;
        int axis;
        if (face == Facing::DOWN || face == Facing::UP)
            axis = AXIS_Y;
        else if (face == Facing::WEST || face == Facing::EAST)
            axis = AXIS_X;
        else // NORTH or SOUTH
            axis = AXIS_Z;
        return woodType | (axis << 2);
    }

protected:
    int getSpawnResourcesAuxValue(int data) {
        // Drop only wood species bits, not the axis bits.
        return data & TYPE_MASK;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__*/
