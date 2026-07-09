#include "LanternRenderer.h"
#include "../Tesselator.h"
#include "../../../world/level/tile/entity/TileEntity.h"
#include "../../../world/level/tile/LanternTile.h"

// x/z footprint is the same floor or ceiling; only the y range moves - see
// LanternTile for the matching collision boxes.
static const float X0 = 0.35f, X1 = 0.65f;
static const float Z0 = 0.35f, Z1 = 0.65f;

// Source rect within lantern.png (16x48) - cropped away from the unrelated
// marks/dead space next to the icon (see Tile.cpp's setCustomIcon() call for
// the flat GUI/hand icon, which uses the whole icon including both bands
// below). The icon has two visually distinct bands: the lit cap+cage
// (rows 0-8) goes on the side faces, and the darker solid base (rows 9-14)
// is a separate texture for the bottom-facing quad only - it was drawn as
// its own piece, not meant to wrap onto the sides with the lit part.
static const float ICON_U0 = 0.0f, ICON_U1 = 6.0f / 16.0f;
static const float SIDE_V0 = 0.0f, SIDE_V1 = 9.0f / 48.0f;
static const float BOTTOM_V0 = 9.0f / 48.0f, BOTTOM_V1 = 15.0f / 48.0f;

void LanternRenderer::render( TileEntity* te, float x, float y, float z, float a )
{
	bool ceiling = te->getData() == LanternTile::CEILING;
	float y0 = ceiling ? 0.25f : 0.0f;
	float y1 = ceiling ? 1.0f  : 0.75f;

	bindTexture("lantern.png");

	Tesselator& t = Tesselator::instance;
	t.colorABGR(0xffffffff);
	t.begin();

	// +Z side
	t.vertexUV(x+X0, y+y0, z+Z1, ICON_U0, SIDE_V1);
	t.vertexUV(x+X1, y+y0, z+Z1, ICON_U1, SIDE_V1);
	t.vertexUV(x+X1, y+y1, z+Z1, ICON_U1, SIDE_V0);
	t.vertexUV(x+X0, y+y1, z+Z1, ICON_U0, SIDE_V0);

	// -Z side
	t.vertexUV(x+X1, y+y0, z+Z0, ICON_U0, SIDE_V1);
	t.vertexUV(x+X0, y+y0, z+Z0, ICON_U1, SIDE_V1);
	t.vertexUV(x+X0, y+y1, z+Z0, ICON_U1, SIDE_V0);
	t.vertexUV(x+X1, y+y1, z+Z0, ICON_U0, SIDE_V0);

	// +X side
	t.vertexUV(x+X1, y+y0, z+Z1, ICON_U0, SIDE_V1);
	t.vertexUV(x+X1, y+y0, z+Z0, ICON_U1, SIDE_V1);
	t.vertexUV(x+X1, y+y1, z+Z0, ICON_U1, SIDE_V0);
	t.vertexUV(x+X1, y+y1, z+Z1, ICON_U0, SIDE_V0);

	// -X side
	t.vertexUV(x+X0, y+y0, z+Z0, ICON_U0, SIDE_V1);
	t.vertexUV(x+X0, y+y0, z+Z1, ICON_U1, SIDE_V1);
	t.vertexUV(x+X0, y+y1, z+Z1, ICON_U1, SIDE_V0);
	t.vertexUV(x+X0, y+y1, z+Z0, ICON_U0, SIDE_V0);

	// Bottom (always the physical bottom of the fixture, i.e. y0 - the floor
	// end when standing, the hanging/open end when ceiling-mounted)
	t.vertexUV(x+X0, y+y0, z+Z0, ICON_U0, BOTTOM_V0);
	t.vertexUV(x+X1, y+y0, z+Z0, ICON_U1, BOTTOM_V0);
	t.vertexUV(x+X1, y+y0, z+Z1, ICON_U1, BOTTOM_V1);
	t.vertexUV(x+X0, y+y0, z+Z1, ICON_U0, BOTTOM_V1);

	t.draw();
}
