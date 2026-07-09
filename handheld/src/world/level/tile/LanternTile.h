#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LanternTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LanternTile_H__

#include "EntityTile.h"

class TileEntity;

// Lantern - floor-standing or ceiling-hung (data value picks which; see
// FLOOR/CEILING below). All of its visuals come from LanternRenderer (its
// own lantern.png, not terrain.png - see EntityTile/getRenderShape()) rather
// than the normal per-chunk block mesh, and it uses a custom two-box
// collision shape (thin hook + wider cage body) instead of a full cube.
class LanternTile: public EntityTile
{
	typedef EntityTile super;
public:
	static const int FLOOR = 0;
	static const int CEILING = 1;

	LanternTile(int id);

	bool isCubeShaped();
	bool isSolidRender();
	int getRenderShape();

	void updateShape(LevelSource* level, int x, int y, int z);
	AABB* getAABB(Level* level, int x, int y, int z);
	AABB getTileAABB(Level* level, int x, int y, int z);
	void addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes);

	bool mayPlace(Level* level, int x, int y, int z, unsigned char face);
	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);
	void neighborChanged(Level* level, int x, int y, int z, int type);

	TileEntity* newTileEntity();
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LanternTile_H__*/
