#include "MapScreen.h"
#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/gles.h"
#include "../../../world/level/Level.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/entity/Mob.h"
#include "../../../util/Mth.h"
#include "../../../platform/input/Keyboard.h"

#include <cmath>
#include <cstdio>

MapScreen::MapScreen()
:	pixelsPerBlock(3.0f),
	panX(0.0f), panZ(0.0f),
	dragging(false),
	lastMouseX(0), lastMouseY(0)
{
}

bool MapScreen::renderGameBehind() {
	return false;
}

void MapScreen::keyPressed(int eventKey) {
	if (eventKey == Keyboard::KEY_M) {
		minecraft->setScreen(NULL);
		return;
	}
	super::keyPressed(eventKey); // Escape also closes, via the base handler
}

bool MapScreen::handleScroll(int dy) {
	pixelsPerBlock *= (dy > 0) ? 1.25f : 0.8f;
	if (pixelsPerBlock < 0.5f) pixelsPerBlock = 0.5f;
	if (pixelsPerBlock > 24.0f) pixelsPerBlock = 24.0f;
	return true;
}

void MapScreen::mouseClicked(int x, int y, int buttonNum) {
	dragging = true;
	lastMouseX = x;
	lastMouseY = y;
}

void MapScreen::mouseReleased(int x, int y, int buttonNum) {
	dragging = false;
}

int MapScreen::tileToColor(Tile* tile) {
	if (tile == NULL) return 0xff101018;
	if (tile == Tile::grass) return 0xff6aa84f;
	if (tile == Tile::dirt || tile == Tile::farmland) return 0xff8b5a2b;
	if (tile == Tile::sand || tile == Tile::sandStone) return 0xffd9c98a;
	if (tile == Tile::water || tile == Tile::calmWater) return 0xff3b5dc9;
	if (tile == Tile::lava || tile == Tile::calmLava) return 0xffcc4400;
	if (tile == (Tile*)Tile::leaves || tile == (Tile*)Tile::leaves_carried) return 0xff3f6b2e;
	if (tile == Tile::treeTrunk) return 0xff6b4423;
	if (tile == Tile::snow || tile == Tile::topSnow) return 0xffffffff;
	if (tile == Tile::ice) return 0xffa0d8ef;
	if (tile == Tile::gravel) return 0xff8d8577;
	if (tile == Tile::clay) return 0xffa1a7b0;
	if (tile == Tile::obsidian || tile == Tile::glowingObsidian) return 0xff1a1a2e;
	if (tile == Tile::unbreakable || tile == Tile::invisible_bedrock) return 0xff2b2b2b;
	if (tile == Tile::rock || tile == Tile::stoneBrick || tile == Tile::stoneBrickSmooth || tile == Tile::mossStone) return 0xff8a8a8a;
	// Man-made blocks (planks, brick, wool, glass, ore, etc.) all fall
	// through to here - a warm, structure-distinct gray so a player's base
	// visibly stands out against natural terrain on the map.
	return 0xffb0a898;
}

int MapScreen::sampleColor(Level* level, int wx, int wz) {
	int64_t key = ((int64_t)wx << 32) ^ (uint32_t)wz;
	std::unordered_map<int64_t, int>::iterator it = colorCache.find(key);
	if (it != colorCache.end()) return it->second;

	// Level::getTopSolidBlock is built for skylight/heightmap purposes and
	// deliberately walks THROUGH water and leaves to find solid ground
	// underneath - wrong for a map, which should show whatever's actually
	// visible looking straight down (water surface, leaf canopy, or solid
	// ground, whichever is topmost). Do our own plain "first non-air block
	// from the top" scan instead. This also never reveals underground
	// caves/caverns, since it always stops at the very first non-air block.
	int y = Level::DEPTH - 1;
	int tileId = 0;
	while (y > 0) {
		tileId = level->getTile(wx, y, wz);
		if (tileId != 0) break;
		y--;
	}
	Tile* tile = (tileId != 0) ? Tile::tiles[tileId] : NULL;
	int color = tileToColor(tile);
	colorCache[key] = color;
	return color;
}

void MapScreen::render(int xm, int ym, float a) {
	if (!minecraft->level || !minecraft->cameraTargetPlayer) return;

	Level* level = minecraft->level;
	Mob* player = minecraft->cameraTargetPlayer;

	// Drag-to-pan: render() is called every frame with the current mouse
	// position regardless of click state, so just diff it against last
	// frame's while a drag is in progress. Screen-pixel delta converts to
	// world blocks by dividing out the current zoom; dragging right should
	// reveal what's to the left, hence the subtraction.
	if (dragging) {
		panX -= (xm - lastMouseX) / pixelsPerBlock;
		panZ -= (ym - lastMouseY) / pixelsPerBlock;
		lastMouseX = xm;
		lastMouseY = ym;
	}

	float viewX = player->x + panX;
	float viewZ = player->z + panZ;

	fill(0, 0, width, height, 0xff0a0a0a);

	const float centerX = width / 2.0f;
	const float centerY = height / 2.0f;

	int centerBX = Mth::floor(viewX);
	int centerBZ = Mth::floor(viewZ);

	int blockRadiusX = (int)(centerX / pixelsPerBlock) + 1;
	int blockRadiusZ = (int)(centerY / pixelsPerBlock) + 1;

	// Batched draws (rather than one fill() call per column, which would be
	// far too slow at per-block resolution) - but the shared Tesselator
	// vertex buffer has a fixed capacity (~87k quads). Zoomed out far
	// enough, this grid can badly exceed that; overflowing it doesn't clip
	// gracefully, it discards the ENTIRE in-progress batch (see
	// Tesselator::vertex's "Overwriting the vertex buffer" path), which is
	// exactly what made the whole map vanish at low zoom. Flush in bounded-
	// size batches instead so no single one can ever overflow it.
	const int MAX_QUADS_PER_BATCH = 10000;
	int quadsInBatch = 0;

	Tesselator& t = Tesselator::instance;
	glDisable2(GL_TEXTURE_2D);
	t.begin();
	for (int bz = -blockRadiusZ; bz <= blockRadiusZ; bz++) {
		int wz = centerBZ + bz;
		int cz = wz >> 4;
		for (int bx = -blockRadiusX; bx <= blockRadiusX; bx++) {
			int wx = centerBX + bx;
			int cx = wx >> 4;
			if (!level->hasChunk(cx, cz)) continue;

			if (quadsInBatch >= MAX_QUADS_PER_BATCH) {
				t.draw();
				t.begin();
				quadsInBatch = 0;
			}

			int color = sampleColor(level, wx, wz);

			float x0 = centerX + (wx - viewX) * pixelsPerBlock;
			float y0 = centerY + (wz - viewZ) * pixelsPerBlock;
			float x1 = x0 + pixelsPerBlock;
			float y1 = y0 + pixelsPerBlock;

			const int abgr = (color & 0xff00ff00) | ((color & 0xff0000) >> 16) | ((color & 0xff) << 16);
			t.colorABGR(abgr);
			t.vertex(x0, y1, 0);
			t.vertex(x1, y1, 0);
			t.vertex(x1, y0, 0);
			t.vertex(x0, y0, 0);
			quadsInBatch++;
		}
	}
	t.draw();
	glEnable2(GL_TEXTURE_2D);

	// Facing-direction arrow, drawn at the player's actual position relative
	// to the (possibly panned/dragged-away) view center - no longer always
	// screen center once panning is in play, and may be off-screen entirely
	// (GL clips it naturally, which is fine). yRot=0 faces +Z (south),
	// increasing clockwise when viewed from above (see Entity::moveRelative)
	// - this matches a north-up map with no axis flip needed.
	float markerX = centerX + (player->x - viewX) * pixelsPerBlock;
	float markerY = centerY + (player->z - viewZ) * pixelsPerBlock;

	float yawRad = player->yRot * Mth::PI / 180.0f;
	float fx = -(float) sin(yawRad);
	float fy = (float) cos(yawRad);
	float rx = fy;
	float ry = -fx;

	const float arrowLen = 10.0f;
	const float arrowBack = 6.0f;
	const float arrowWidth = 6.0f;

	float tipX = markerX + fx * arrowLen;
	float tipY = markerY + fy * arrowLen;
	float leftX = markerX - fx * arrowBack + rx * arrowWidth;
	float leftY = markerY - fy * arrowBack + ry * arrowWidth;
	float rightX = markerX - fx * arrowBack - rx * arrowWidth;
	float rightY = markerY - fy * arrowBack - ry * arrowWidth;

	glDisable2(GL_TEXTURE_2D);
	t.begin(GL_TRIANGLES);
	t.colorABGR(0xffffffff);
	t.vertex(tipX, tipY, 0);
	t.vertex(leftX, leftY, 0);
	t.vertex(rightX, rightY, 0);
	t.draw();
	glEnable2(GL_TEXTURE_2D);

	char buf[64];
	sprintf(buf, "X: %.0f  Z: %.0f", viewX, viewZ);
	font->drawShadow(buf, 4, 4, 0xffffff);
	font->drawShadow("M / Esc to close, scroll to zoom, drag to pan", 4, height - 12, 0xffffff);
}
