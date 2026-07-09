#include "LanternTile.h"
#include "entity/TileEntity.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../Facing.h"

// Envelope/collision shapes (x/z footprint is the same either way, only the
// y range moves): floor-standing sits on the ground (body 0..0.65, hook
// 0.65..0.75); ceiling-hung mirrors that against the top of the space (hook
// 0.9..1.0 flush against the block above, body 0.25..0.9 hanging below it).
static const float FOOT_X0 = 0.35f, FOOT_X1 = 0.65f;
static const float FOOT_Z0 = 0.35f, FOOT_Z1 = 0.65f;
static const float HOOK_X0 = 0.45f, HOOK_X1 = 0.55f;
static const float HOOK_Z0 = 0.45f, HOOK_Z1 = 0.55f;

LanternTile::LanternTile( int id )
:	super(id, Material::metal)
{
	setShape(FOOT_X0, 0, FOOT_Z0, FOOT_X1, 0.75f, FOOT_Z1);
}

bool LanternTile::isCubeShaped() {
	return false;
}

bool LanternTile::isSolidRender() {
	return false;
}

int LanternTile::getRenderShape() {
	return Tile::SHAPE_INVISIBLE;
}

void LanternTile::updateShape( LevelSource* level, int x, int y, int z ) {
	if (level->getData(x, y, z) == CEILING)
		setShape(FOOT_X0, 0.25f, FOOT_Z0, FOOT_X1, 1.0f, FOOT_Z1);
	else
		setShape(FOOT_X0, 0, FOOT_Z0, FOOT_X1, 0.75f, FOOT_Z1);
}

AABB* LanternTile::getAABB( Level* level, int x, int y, int z ) {
	updateShape(level, x, y, z);
	return super::getAABB(level, x, y, z);
}

AABB LanternTile::getTileAABB( Level* level, int x, int y, int z ) {
	updateShape(level, x, y, z);
	return super::getTileAABB(level, x, y, z);
}

void LanternTile::addAABBs( Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes ) {
	if (level->getData(x, y, z) == CEILING) {
		setShape(FOOT_X0, 0.25f, FOOT_Z0, FOOT_X1, 0.9f, FOOT_Z1);
		super::addAABBs(level, x, y, z, box, boxes);

		setShape(HOOK_X0, 0.9f, HOOK_Z0, HOOK_X1, 1.0f, HOOK_Z1);
		super::addAABBs(level, x, y, z, box, boxes);
	} else {
		setShape(FOOT_X0, 0, FOOT_Z0, FOOT_X1, 0.65f, FOOT_Z1);
		super::addAABBs(level, x, y, z, box, boxes);

		setShape(HOOK_X0, 0.65f, HOOK_Z0, HOOK_X1, 0.75f, HOOK_Z1);
		super::addAABBs(level, x, y, z, box, boxes);
	}

	updateShape(level, x, y, z);
}

bool LanternTile::mayPlace( Level* level, int x, int y, int z, unsigned char face ) {
	return level->isSolidBlockingTile(x, y - 1, z) || level->isSolidBlockingTile(x, y + 1, z);
}

int LanternTile::getPlacedOnFaceDataValue( Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue ) {
	// Clicking straight up into the underside of a block hangs it from that
	// block; clicking straight down onto a block's top stands it on that
	// block. A side click falls back to whichever support actually exists.
	if (face == Facing::DOWN) return CEILING;
	if (face == Facing::UP) return FLOOR;
	if (level->isSolidBlockingTile(x, y - 1, z)) return FLOOR;
	return CEILING;
}

void LanternTile::neighborChanged( Level* level, int x, int y, int z, int type ) {
	int data = level->getData(x, y, z);
	bool supported = (data == CEILING)
		? level->isSolidBlockingTile(x, y + 1, z)
		: level->isSolidBlockingTile(x, y - 1, z);

	if (!supported) {
		spawnResources(level, x, y, z, data);
		level->setTile(x, y, z, 0);
	}
}

TileEntity* LanternTile::newTileEntity() {
	return TileEntityFactory::createTileEntity(TileEntityType::Lantern);
}
