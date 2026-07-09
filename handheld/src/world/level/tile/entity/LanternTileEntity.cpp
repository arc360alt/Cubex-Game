#include "LanternTileEntity.h"

LanternTileEntity::LanternTileEntity()
:	super(TileEntityType::Lantern)
{
	rendererId = TR_LANTERN_RENDERER;
}

bool LanternTileEntity::shouldSave() {
	return false;
}
