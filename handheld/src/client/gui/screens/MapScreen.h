#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__MapScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__MapScreen_H__

#include "../Screen.h"
#include <unordered_map>
#include <cstdint>

class Tile;

// Fullscreen top-down overview of the world around the player, with an
// arrow showing facing direction. Opened with M (see Minecraft.cpp's
// keyboard loop), closed with M or Escape. Samples one color per BLOCK
// column (not per chunk), so actual structures are recognizable - scroll
// wheel zooms (changes how many screen pixels each block occupies, not the
// sample resolution, so it stays sharp at any zoom level). Column colors
// are cached per-session (cleared each time the screen is (re)opened, since
// it's a fresh object each time) so re-drawing an unmoving view across
// frames doesn't re-scan the world every frame.
class MapScreen: public Screen
{
	typedef Screen super;
public:
	MapScreen();

	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void keyPressed(int eventKey);
	bool handleScroll(int dy);

protected:
	void mouseClicked(int x, int y, int buttonNum);
	void mouseReleased(int x, int y, int buttonNum);

private:
	static int tileToColor(Tile* tile);
	int sampleColor(class Level* level, int wx, int wz);

	float pixelsPerBlock;
	std::unordered_map<int64_t, int> colorCache;

	// World-space offset (in blocks) from the player's position that the
	// view is centered on - lets the map be dragged around independently
	// of where the player actually is. Reset to (0,0) on open.
	float panX, panZ;
	bool dragging;
	int lastMouseX, lastMouseY;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__MapScreen_H__*/
