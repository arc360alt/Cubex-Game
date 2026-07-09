#include "TrapDoorTile.h"
#include "../material/Material.h"
#include "../Level.h"
#include "LevelEvent.h"
#include "../../Facing.h"
#include <cmath>
TrapDoorTile::TrapDoorTile( int id, const Material* material, int texOverride ) : super(id, material) {
	if (texOverride >= 0) {
		tex = texOverride;
	} else {
		tex = 4 + 5 * 16;
		if(material == Material::metal) tex++;
	}
	float r = 0.5f;
	float h = 1.0f;
	super::setShape(0.5f - r, 0, 0.5f - r, 0.5f + r, h, 0.5f + r);
}

bool TrapDoorTile::blocksLight() {
	return false;
}

bool TrapDoorTile::isSolidRender() {
	return false;
}

bool TrapDoorTile::isCubeShaped() {
	return false;
}

int TrapDoorTile::getRenderShape() {
	return Tile::SHAPE_BLOCK;
}

int TrapDoorTile::getRenderLayer() {
	return Tile::RENDERLAYER_ALPHATEST;
}

AABB TrapDoorTile::getTileAABB( Level* level, int x, int y, int z ) {
	updateShape(level, x, y, z);
	return super::getTileAABB(level, x, y, z);
}

AABB* TrapDoorTile::getAABB( Level* level, int x, int y, int z ) {
	updateShape(level, x, y, z);
	return super::getAABB(level, x, y, z);
}

void TrapDoorTile::updateShape( LevelSource* level, int x, int y, int z ) {
	setShape(level->getData(x, y, z));
}

void TrapDoorTile::updateDefaultShape() {
	float r = 3 / 16.0f;
	super::setShape(0, 0.5f - r / 2, 0, 1, 0.5f + r / 2, 1);
}

void TrapDoorTile::setShape( int data ) {
	float r = 3 / 16.0f;
	if (isOpen(data)) {
		// Open shape is always fully vertical against the hinge wall,
		// regardless of TOP_HALF - matches vanilla (a top-half and a
		// bottom-half trapdoor look identical once swung open).
		if ((data & 3) == 0) super::setShape(0, 0, 1 - r, 1, 1, 1);
		if ((data & 3) == 1) super::setShape(0, 0, 0, 1, 1, r);
		if ((data & 3) == 2) super::setShape(1 - r, 0, 0, 1, 1, 1);
		if ((data & 3) == 3) super::setShape(0, 0, 0, r, 1, 1);
	} else if (data & TOP_HALF) {
		super::setShape(0, 1 - r, 0, 1, 1, 1);
	} else {
		super::setShape(0, 0, 0, 1, r, 1);
	}
}

void TrapDoorTile::attack( Level* level, int x, int y, int z, Player* player ) {
	use(level, x, y, z, player);
}

bool TrapDoorTile::use( Level* level, int x, int y, int z, Player* player ) {
	if (material == Material::metal) return true;

	int dir = level->getData(x, y, z);
	level->setData(x, y, z, dir ^ 4);

	level->levelEvent(player, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);
	return true;
}

void TrapDoorTile::setOpen( Level* level, int x, int y, int z, bool shouldOpen ) {
	int dir = level->getData(x, y, z);

	bool wasOpen = (dir & 4) > 0;
	if (wasOpen == shouldOpen) return;

	level->setData(x, y, z, dir ^ 4);

	level->levelEvent(NULL, LevelEvent::SOUND_OPEN_DOOR, x, y, z, 0);
}

void TrapDoorTile::neighborChanged( Level* level, int x, int y, int z, int type ) {
	if (level->isClientSide) return;

	int data = level->getData(x, y, z);

	// A trapdoor is valid if it's still resting on a block below (how a
	// face==UP/"floor" placement supports itself) OR still has its hinge
	// wall (how a side-face placement supports itself) - TOP_HALF only
	// changes which end of the space it renders in, not what supports it,
	// so both checks apply regardless of that bit.
	int xt = x;
	int zt = z;
	if ((data & 3) == 0) zt++;
	if ((data & 3) == 1) zt--;
	if ((data & 3) == 2) xt++;
	if ((data & 3) == 3) xt--;

	bool supportedBelow = attachesTo(level->getTile(x, y - 1, z));
	bool supportedBySide = attachesTo(level->getTile(xt, y, zt));

	if (!supportedBelow && !supportedBySide) {
		level->setTile(x, y, z, 0);
		popResource(level, x, y, z, ItemInstance(Tile::trapdoor));
	}
}

HitResult TrapDoorTile::clip( Level* level, int xt, int yt, int zt, const Vec3& a, const Vec3& b ) {
	updateShape(level, xt, yt, zt);
	return super::clip(level, xt, yt, zt, a, b);
}

int TrapDoorTile::getDir( int dir ) {
	if ((dir & 4) == 0) {
		return ((dir - 1) & 3);
	} else {
		return (dir & 3);
	}
}

int TrapDoorTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) {
	if (face == Facing::UP) {
		// No wall to hinge to - this trapdoor is self-supporting on the block
		// below, so it always sits at the bottom of its own space, flush on
		// top of that block (like a slab placed on top always comes out as
		// the bottom half). dir here is purely which way it visually swings.
		float dx = clickX - 0.5f;
		float dz = clickZ - 0.5f;
		int dir;
		if (fabsf(dz) >= fabsf(dx))
			dir = (dz >= 0) ? 0 : 1;
		else
			dir = (dx >= 0) ? 2 : 3;
		return dir;
	}

	int dir = 0;
	if (face == Facing::NORTH) dir = 0;
	if (face == Facing::SOUTH) dir = 1;
	if (face == Facing::WEST)  dir = 2;
	if (face == Facing::EAST)  dir = 3;

	// Mirrors StoneSlabTile: clicking the upper half of a side face places
	// this trapdoor flush against the top of its own space instead of the
	// bottom, same wall/hinge either way.
	if (clickY > 0.5f) dir |= TOP_HALF;
	return dir;
}

bool TrapDoorTile::mayPlace( Level* level, int x, int y, int z, unsigned char face) {
	if (face == 0) return false;
	if (face == 1) return attachesTo(level->getTile(x, y - 1, z));
	if (face == 2) z++;
	if (face == 3) z--;
	if (face == 4) x++;
	if (face == 5) x--;

	return attachesTo(level->getTile(x, y, z));
}

bool TrapDoorTile::isOpen( int data ) {
	return (data & 4) != 0;
}

bool TrapDoorTile::attachesTo( int id ) {
	if (id <= 0) {
		return false;
	}
	Tile* tile = Tile::tiles[id];
	bool isStair = tile != NULL && tile->getRenderShape() == Tile::SHAPE_STAIRS;
	return tile != NULL && (tile->material->isSolidBlocking() && tile->isCubeShaped()) || tile == Tile::lightGem || tile == Tile::stoneSlabHalf || isStair;
}
