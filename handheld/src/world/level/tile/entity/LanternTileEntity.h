#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__LanternTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__LanternTileEntity_H__

#include "TileEntity.h"

// Purely a rendering hook (see LanternRenderer) - the lantern has no state
// of its own, so there's nothing to save/load.
class LanternTileEntity: public TileEntity
{
	typedef TileEntity super;
public:
	LanternTileEntity();

	bool shouldSave();
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__LanternTileEntity_H__*/
