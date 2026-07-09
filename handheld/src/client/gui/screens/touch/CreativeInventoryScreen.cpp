#include "CreativeInventoryScreen.h"

#include "../../../Minecraft.h"
#include "../../Gui.h"
#include "../../../renderer/gles.h"
#include "../../../renderer/entity/ItemRenderer.h"
#include "../../../renderer/Tesselator.h"
#include "../../../renderer/Textures.h"
#include "../../../sound/SoundEngine.h"
#include "../../../player/LocalPlayer.h"
#include "../../../../world/entity/player/Inventory.h"
#include "../../../../world/entity/player/Player.h"
#include "../../../../world/item/Item.h"
#include "../../../../world/item/ItemInstance.h"
#include "../../../../world/item/DyePowderItem.h"
#include "../../../../world/level/tile/Tile.h"
#include "../../../../world/level/tile/StoneSlabTile.h"
#include "../../../../world/level/tile/QuartzBlockTile.h"

#include <set>

namespace Touch {

static const float BorderPixels = 4;
static const float BlockPixels = 24;
static const int ItemSize = (int)(BlockPixels + 2 * BorderPixels);
static const int Bx = 10;
static const int By = 6;

//
// CatalogPane - renders/handles clicks for the standalone catalog grid.
// Deliberately never enters Tesselator's override-batching mode, so
// ItemRenderer::renderGuiItem always takes its "not overridden" branch and
// draws the item directly - no red-square fallback flash is possible here.
//
CreativeInventoryScreen::CatalogPane::CatalogPane(CreativeInventoryScreen* owner, Minecraft* mc, const IntRectangle& rect, int columns, int numItems, float screenScale, int itemBorderPixels)
:	super(SF_LockX | SF_ShowScrollbar | SF_NoHoldSelect,
		rect,
		IntRectangle(0, 0, ItemSize, ItemSize),
		columns, numItems, screenScale),
	owner(owner),
	mc(mc),
	itemBorderPixels(itemBorderPixels),
	lastClickedIndex(-1),
	lastClickedTicks(-1)
{
	clickArea = new RectangleArea((float)rect.x, (float)rect.y, (float)(rect.x + rect.w), (float)(rect.y + rect.h));
}

CreativeInventoryScreen::CatalogPane::~CatalogPane() {
	delete clickArea;
}

void CreativeInventoryScreen::CatalogPane::renderBatch(std::vector<GridItem>& items, float alpha) {
	fill((float)(bbox.x - 2), (float)(bbox.y - 4), (float)(bbox.x + bbox.w + 2), (float)(bbox.y + bbox.h + 4), 0xff333333);

	glEnable2(GL_BLEND);
	glDisable2(GL_ALPHA_TEST);
	glEnable2(GL_SCISSOR_TEST);
	GLuint sx = (GLuint)(screenScale * bbox.x);
	GLuint sy = mc->height - (GLuint)(screenScale * (bbox.y + bbox.h));
	GLuint sw = (GLuint)(screenScale * bbox.w);
	GLuint sh = (GLuint)(screenScale * bbox.h);
	glScissor(sx, sy, sw, sh);

	Tesselator& t = Tesselator::instance;
	glEnable2(GL_TEXTURE_2D);
	glColor4f2(1, 1, 1, 1);
	mc->textures->loadAndBindTexture("gui/gui.png");
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		blit((int)item.xf, (int)item.yf, 200, 46, itemBbox.w, itemBbox.h, 16, 16);
	}

	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		int catalogIndex = item.id;
		const ItemInstance* citem = owner->getCatalogItem(catalogIndex);
		if (!citem) continue;

		if (lastClickedTicks > 0 && lastClickedIndex == catalogIndex) {
			int gv = 255 - lastClickedTicks * 15;
			glColor4f2(gv / 255.0f, gv / 255.0f, gv / 255.0f, 1.0f);
		} else {
			glColor4f2(1, 1, 1, 1);
		}

		float xx = Gui::floorAlignToScreenPixel(item.xf + itemBorderPixels + 4);
		float yy = Gui::floorAlignToScreenPixel(item.yf + itemBorderPixels + 4);
		mc->textures->loadAndBindTexture("terrain.png");
		ItemRenderer::renderGuiItem(NULL, mc->textures, citem, xx, yy, 16, 16, false);
	}

	glDisable2(GL_SCISSOR_TEST);

	fillGradient((float)(bbox.x - 2), (float)bbox.y, (float)(bbox.x + bbox.w + 2), (float)(bbox.y + 20), 0x99000000, 0x00000000);
	fillGradient((float)(bbox.x - 2), (float)(bbox.y + bbox.h - 20), (float)(bbox.x + bbox.w + 2), (float)(bbox.y + bbox.h), 0x00000000, 0x99000000);

	if (vScroll.alpha > 0) {
		const int color = ((int)(255.0f * vScroll.alpha) << 24) | 0xaaaaaa;
		const float xx = (float)(bbox.x + bbox.w);
		fill(xx - vScroll.w, vScroll.y, xx, vScroll.y + vScroll.h, color);
	}
}

bool CreativeInventoryScreen::CatalogPane::onSelect(int gridId, bool selected) {
	if (gridId < 0 || gridId >= owner->getCatalogSize())
		return false;

	owner->onCatalogItemClicked(gridId);
	lastClickedIndex = gridId;
	lastClickedTicks = 7;
	return false;
}

//
// CreativeInventoryScreen
//
CreativeInventoryScreen::CreativeInventoryScreen()
:	_pane(NULL),
	_columns(0),
	bHeader(0, "Select item"),
	bDone(1, "")
{
}

CreativeInventoryScreen::~CreativeInventoryScreen() {
	delete _pane;
	freeCatalog();
}

void CreativeInventoryScreen::freeCatalog() {
	for (unsigned int i = 0; i < _catalog.size(); ++i)
		delete _catalog[i];
	_catalog.clear();
}

const ItemInstance* CreativeInventoryScreen::getCatalogItem(int index) const {
	if (index < 0 || index >= (int)_catalog.size()) return NULL;
	return _catalog[index];
}

void CreativeInventoryScreen::buildCatalog() {
	freeCatalog();

	// Tiles/items whose creative entry is expanded into several specific
	// metadata variants below rather than a single generic (aux=0) entry.
	std::set<int> expandVariants;
	expandVariants.insert(Tile::cloth->id);
	expandVariants.insert(Tile::sapling->id);
	expandVariants.insert(Tile::treeTrunk->id);
	expandVariants.insert(((Tile*)Tile::leaves)->id);
	expandVariants.insert(Tile::sandStone->id);
	expandVariants.insert(Tile::stoneBrickSmooth->id);
	expandVariants.insert(Tile::quartzBlock->id);
	expandVariants.insert(Tile::stoneSlabHalf->id);

	for (int id = 1; id < Tile::NUM_BLOCK_TYPES; ++id) {
		Tile* tile = Tile::tiles[id];
		if (!tile) continue;
		if (tile->category < 0) continue;
		if (expandVariants.find(tile->id) != expandVariants.end()) continue;
		_catalog.push_back(new ItemInstance(tile));
	}

	for (int d = 0; d < 16; ++d)
		_catalog.push_back(new ItemInstance(Tile::cloth, 1, d));
	for (int d = 0; d < 3; ++d) {
		_catalog.push_back(new ItemInstance(Tile::sapling, 1, d));
		_catalog.push_back(new ItemInstance(Tile::treeTrunk, 1, d));
		_catalog.push_back(new ItemInstance((Tile*)Tile::leaves, 1, d));
		_catalog.push_back(new ItemInstance(Tile::sandStone, 1, d));
		_catalog.push_back(new ItemInstance(Tile::stoneBrickSmooth, 1, d));
		_catalog.push_back(new ItemInstance(Tile::quartzBlock, 1, d));
	}
	for (int d = 0; d <= StoneSlabTile::SPRUCE_SLAB; ++d)
		_catalog.push_back(new ItemInstance(Tile::stoneSlabHalf, 1, d));

	for (int id = 256; id < Item::MAX_ITEMS; ++id) {
		Item* item = Item::items[id];
		if (!item) continue;
		if (item->category < 0) continue;
		if (item == Item::dye_powder) continue;
		_catalog.push_back(new ItemInstance(item));
	}
	for (int d = 0; d < 16; ++d)
		_catalog.push_back(new ItemInstance(Item::dye_powder, 1, d));

	for (unsigned int i = 0; i < _catalog.size(); ++i)
		_catalog[i]->count = 1;
}

void CreativeInventoryScreen::init()
{
	buildCatalog();

	int maxWidth = width - Bx - Bx;
	_columns = maxWidth / ItemSize;
	if (_columns < 1) _columns = 1;
	const int realWidth = _columns * ItemSize;
	const int realBx = (width - realWidth) / 2;

	_rect = IntRectangle(realBx, 24 + By, realWidth, height - By - By - 20 - 24);
	const int topListMargin = 6;
	_rect.y += topListMargin;
	_rect.h -= topListMargin;
	const int paneInset = 2;
	_rect.x += paneInset;
	_rect.y += paneInset;
	_rect.w -= paneInset * 2;
	_rect.h -= paneInset * 2;

	_pane = new CatalogPane(this, minecraft, _rect, _columns, (int)_catalog.size(), Gui::GuiScale, (int)BorderPixels);

	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	bDone.setImageDef(def, true);
	bDone.width = bDone.height = 19;
	bDone.scaleWhenPressed = false;

	buttons.push_back(&bHeader);
	buttons.push_back(&bDone);
}

void CreativeInventoryScreen::setupPositions() {
	bHeader.y = bDone.y = 0;
	bDone.x = width - bDone.width;
	bHeader.x = 0;
	bHeader.width = width;
	bHeader.xText = width / 2;

	clippingArea.x = 0;
	clippingArea.w = width;
	clippingArea.y = 0;
	clippingArea.h = (int)(Gui::GuiScale * 24);
}

void CreativeInventoryScreen::removed() {
	minecraft->gui.inventoryUpdated();
}

void CreativeInventoryScreen::tick() {
	_pane->tick();
	super::tick();
}

void CreativeInventoryScreen::render(int xm, int ym, float a) {
	glDisable2(GL_DEPTH_TEST);
	glEnable2(GL_BLEND);

	// No full-screen background fill here on purpose: GameRenderer calls
	// Gui::render() (hotbar, health, etc.) every frame *before* Screen::render(),
	// so that HUD is already sitting behind whatever this draws. A solid fill
	// would blot it out along with the sky. The old survival screen doesn't
	// fill the background either, for the same reason - showing the world/
	// hotbar behind the item list is the intended look here, not a bug.

	Screen::render(xm, ym, a);
	_pane->render(xm, ym, a);

	glEnable2(GL_TEXTURE_2D);
	minecraft->textures->loadAndBindTexture("gui/itemframe.png");
	glEnable2(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blit(0, _rect.y - By, 0, 0, width, _rect.h + By + By, 215, 256);
	glDisable2(GL_BLEND);

	glEnable2(GL_DEPTH_TEST);
}

bool CreativeInventoryScreen::hasClippingArea(IntRectangle& out) {
	out = clippingArea;
	return true;
}

void CreativeInventoryScreen::mouseClicked(int x, int y, int buttonNum) {
	super::mouseClicked(x, y, buttonNum);
}

void CreativeInventoryScreen::mouseReleased(int x, int y, int buttonNum) {
	super::mouseReleased(x, y, buttonNum);
}

void CreativeInventoryScreen::buttonClicked(Button* button) {
	if (button->id == bDone.id)
		minecraft->setScreen(NULL);
}

void CreativeInventoryScreen::onCatalogItemClicked(int catalogIndex) {
	const ItemInstance* citem = getCatalogItem(catalogIndex);
	if (!citem) return;

	Inventory* inventory = minecraft->player->inventory;
	inventory->giveItemToSelectedSlot(*citem);

	minecraft->soundEngine->playUI("random.pop2", 1.0f, 0.3f);
	minecraft->gui.flashSlot(inventory->selected);
}

}
