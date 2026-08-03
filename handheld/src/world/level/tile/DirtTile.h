#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__

//package net.minecraft.world.level.tile;

#include "../material/Material.h"
#include "../Level.h"
#include "../LevelSource.h"

#include "Tile.h"

class DirtTile: public Tile
{
	typedef Tile super;
public:
    // Growth is tracked in this tile's data nibble (0-15, unused by dirt
    // otherwise). Each step only advances while grass remains adjacent;
    // losing the neighbor resets progress to 0 ("disrupted").
    static const int GROW_STEPS = 15;
    static const int GROW_STEP_DELAY = 80; // ticks between steps (~4s @ 20 tps) -> 15 steps = ~60s max

    /*protected*/
	DirtTile(int id, int tex)
	:	Tile(id, tex, Material::dirt)
    {
        setTicking(true);
	}

    // Level::addToTickNextTick() runs the tile's tick() SYNCHRONOUSLY,
    // right then and there, instead of deferring it, whenever
    // level->instaTick is set (SpringFeature flips it on briefly during
    // world-gen spring placement). tick() below both reschedules itself
    // (self-recursion) and, via setTile(grass)'s neighbor-update cascade,
    // can trigger neighborChanged on adjacent dirt (cross-block recursion)
    // - under instaTick that unwinds as an unbounded synchronous C++ call
    // stack across an entire connected dirt/grass field instead of the
    // normal spread-over-real-ticks behavior, which stack-overflows on any
    // sizeable flat grassland. Refusing to schedule at all while instaTick
    // is active sidesteps this entirely; the growth timer just resumes
    // normally (via the lottery or the next real neighbor change) once the
    // brief instaTick window ends, with no visible behavior loss.
    void neighborChanged(Level* level, int x, int y, int z, int type) {
        if (level->isClientSide || level->instaTick) return;
        if (level->getData(x, y, z) == 0 && hasGrassNeighbor(level, x, y, z)) {
            level->addToTickNextTick(x, y, z, id, GROW_STEP_DELAY);
        }
    }

    void onPlace(Level* level, int x, int y, int z) {
        if (level->isClientSide || level->instaTick) return;
        if (hasGrassNeighbor(level, x, y, z)) {
            level->addToTickNextTick(x, y, z, id, GROW_STEP_DELAY);
        }
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->isClientSide) return;

        if (!hasGrassNeighbor(level, x, y, z)) {
            if (level->getData(x, y, z) != 0) level->setData(x, y, z, 0);
            return;
        }

        int progress = level->getData(x, y, z);
        if (progress >= GROW_STEPS) {
            level->setTile(x, y, z, Tile::grass->id);
            return;
        }
        if (level->instaTick) return; // don't reschedule into a synchronous recursive call
        level->setData(x, y, z, progress + 1);
        level->addToTickNextTick(x, y, z, id, GROW_STEP_DELAY);
    }

    static bool hasGrassNeighbor(LevelSource* level, int x, int y, int z) {
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (dx == 0 && dz == 0) continue;
                if (level->getTile(x + dx, y, z + dz) == Tile::grass->id) return true;
                if (level->getTile(x + dx, y + 1, z + dz) == Tile::grass->id) return true;
            }
        }
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__*/
