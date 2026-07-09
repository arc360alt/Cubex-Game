#include "ArmorScreen.h"
#include "crafting/WorkbenchScreen.h"
#include "IngameBlockSelectionScreen.h"
#include "ScreenChooser.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../util/Mth.h"
#include "../Screen.h"
#include "../components/NinePatch.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ItemCategory.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/entity/item/ItemEntity.h"
#include "../../../world/level/Level.h"
#include "../../../network/RakNetInstance.h"
#include "../../renderer/entity/EntityRenderDispatcher.h"
#include "../../../world/item/ArmorItem.h"

static void setIfNotSet(bool& ref, bool condition) {
	ref = (ref || condition);
}

const int   descFrameWidth = 100;

const int rgbActive = 0xfff0f0f0;
const int rgbInactive = 0xc0635558;
const int rgbInactiveShadow = 0xc0aaaaaa;

#ifdef __APPLE__
	static const float BorderPixels = 4;
	#ifdef DEMO_MODE
		static const float BlockPixels = 22;
	#else
		static const float BlockPixels = 22;
	#endif
#else
	static const float BorderPixels = 4;
	static const float BlockPixels = 24;
#endif
static const int ItemSize = (int)(BlockPixels + 2*BorderPixels);

static const int Bx = 10; // Border Frame width
static const int By = 6; // Border Frame height


ArmorScreen::ArmorScreen():
	inventoryPane(NULL),
	btnArmor0(0),
	btnArmor1(1),
	btnArmor2(2),
	btnArmor3(3),
	btnClose(4, ""),
	bHeader (5, "Inventory"),
	btnCraft(6, "Craft"),
	btnItems(7, "Items"),
	guiBackground(NULL),
	guiSlot(NULL),
	guiPaneFrame(NULL),
	guiPlayerBg(NULL),
	doRecreatePane(false),
	cursorItem(),
	descWidth(90)
{
}

ArmorScreen::~ArmorScreen() {
	delete inventoryPane;

	delete guiBackground;
	delete guiSlot;
	delete guiPaneFrame;
	delete guiPlayerBg;
}

void ArmorScreen::init() {
	super::init();

	player = minecraft->player;

	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	btnClose.setImageDef(def, true);
	btnClose.scaleWhenPressed = false;

	buttons.push_back(&bHeader);
	buttons.push_back(&btnClose);
	buttons.push_back(&btnCraft);
	buttons.push_back(&btnItems);

	armorButtons[0] = &btnArmor0;
	armorButtons[1] = &btnArmor1;
	armorButtons[2] = &btnArmor2;
	armorButtons[3] = &btnArmor3;
	for (int i = 0; i < NUM_ARMORBUTTONS; ++i)
		buttons.push_back(armorButtons[i]);

	// GUI - nine patches
	NinePatchFactory builder(minecraft->textures, "gui/spritesheet.png");

	guiBackground   = builder.createSymmetrical(IntRectangle(0, 0, 16, 16), 4, 4);
	guiSlot         = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 3, 3, 20, 20);
	guiPaneFrame    = builder.createSymmetrical(IntRectangle(28, 42, 4, 4), 1, 1)->setExcluded(1 << 4);
	guiPlayerBg     = builder.createSymmetrical(IntRectangle(0, 20, 8, 8), 3, 3);

	updateItems();
}

void ArmorScreen::setupPositions() {
	// Left  - Categories
	bHeader.x = bHeader.y = 0;
	bHeader.width = width;

	btnClose.width = btnClose.height = 19;
	btnClose.x = width - btnClose.width;
	btnClose.y = 0;

	btnCraft.height = 19;
	btnCraft.width  = 44;
	btnCraft.x = btnClose.x - btnCraft.width - 2;
	btnCraft.y = 0;

	btnItems.height = 19;
	btnItems.width  = 44;
	btnItems.x = btnCraft.x - btnItems.width - 2;
	btnItems.y = 0;

	// Inventory pane — reserve ItemSize+4 pixels at the bottom for the hotbar row
	const int maxWidth = (int)(width/1.8f) - Bx - Bx;
	const int InventoryColumns = maxWidth / ItemSize;
	const int realWidth = InventoryColumns * ItemSize;
	const int paneWidth = realWidth + Bx + Bx;
	const int realBx = (paneWidth - realWidth) / 2;
	const int hotbarReserve = ItemSize + 4;

	inventoryPaneRect = IntRectangle(realBx,
#ifdef __APPLE__
		26 + By - ((width==240)?1:0), realWidth, ((width==240)?1:0) + height-By-By-28 - hotbarReserve);
#else
		26 + By, realWidth, height-By-By-28 - hotbarReserve);
#endif

	// Hotbar row: 9 slots, centred under the full screen
	const int hotbarTotalW = NUM_HOTBAR * ItemSize;
	hotbarRect.x = (width - hotbarTotalW) / 2;
	hotbarRect.y = inventoryPaneRect.y + inventoryPaneRect.h + 2;
	hotbarRect.w = hotbarTotalW;
	hotbarRect.h = ItemSize;

	for (int i = 0; i < NUM_ARMORBUTTONS; ++i) {
		Button& b = *armorButtons[i];
		b.x = paneWidth;
        b.y = inventoryPaneRect.y + 24 * i;
		b.width = 20;
		b.height = 20;
	}

	guiPlayerBgRect.y = inventoryPaneRect.y;
	int xx = armorButtons[0]->x + armorButtons[0]->width;
	int xw = width - xx;
	guiPlayerBgRect.x = xx + xw / 10;
	guiPlayerBgRect.w = xw - (xw / 10) * 2;
	guiPlayerBgRect.h = inventoryPaneRect.h;

	guiPaneFrame->setSize((float)inventoryPaneRect.w + 2, (float)inventoryPaneRect.h + 2);
	guiPlayerBg->setSize((float)guiPlayerBgRect.w, (float)guiPlayerBgRect.h);
	guiBackground->setSize((float)width, (float)height);

	updateItems();
	setupInventoryPane();
}

void ArmorScreen::tick() {
	if (inventoryPane)
		inventoryPane->tick();

	if (doRecreatePane) {
		updateItems();
		setupInventoryPane();
		doRecreatePane = false;
	}
}

void ArmorScreen::handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a) {
	if (pane) {
		pane->render(xm, ym, a);
		guiPaneFrame->draw(t, (float)(pane->rect.x - 1), (float)(pane->rect.y - 1));
	}
}

void ArmorScreen::render(int xm, int ym, float a) {
	//renderBackground();

	Tesselator& t = Tesselator::instance;

    t.addOffset(0, 0, -500);
	guiBackground->draw(t, 0, 0);
    t.addOffset(0, 0, 500);
	glEnable2(GL_ALPHA_TEST);

	// Buttons (Left side + crafting)
	super::render(xm, ym, a);

	handleRenderPane(inventoryPane, t, xm, ym, a);

	t.colorABGR(0xffffffff);
	glColor4f2(1, 1, 1, 1);

	t.addOffset(0, 0, -490);
	guiPlayerBg->draw(t, (float)guiPlayerBgRect.x, (float)guiPlayerBgRect.y);
	t.addOffset(0, 0, 490);
	{
		float xo = (float)(guiPlayerBgRect.x + guiPlayerBgRect.w / 2);
		// Model sits above yo; centre offset = 45*(heightOffset+0.9) = 113.4 px
		float yo = (float)(guiPlayerBgRect.y + guiPlayerBgRect.h / 2) + 113.4f;
		renderPlayer(xo, yo);
	}

	for (int i = 0; i < NUM_ARMORBUTTONS; ++i) {
		drawSlotItemAt(t, i, player->getArmor(i), armorButtons[i]->x, armorButtons[i]->y);
	}

	// Hotbar row — draw 9 slots, then selection outline on top
	{
		Inventory* inv = minecraft->player->inventory;
		for (int i = 0; i < NUM_HOTBAR; ++i) {
			int sx = hotbarRect.x + i * ItemSize;
			int sy = hotbarRect.y;
			drawSlotItemAt(t, -1, inv->getLinked(i), sx, sy);
		}
		// 1px white border around the selected slot, drawn after all slots/items
		int sel = inv->selected;
		if (sel >= 0 && sel < NUM_HOTBAR) {
			float sx = (float)(hotbarRect.x + sel * ItemSize);
			float sy = (float)hotbarRect.y;
			float ex = sx + ItemSize, ey = sy + ItemSize;
			fill(sx-1, sy-1, ex+1, sy,   0xffffffff); // top
			fill(sx-1, ey,   ex+1, ey+1, 0xffffffff); // bottom
			fill(sx-1, sy,   sx,   ey,   0xffffffff); // left
			fill(ex,   sy,   ex+1, ey,   0xffffffff); // right
		}
	}

	glDisable2(GL_ALPHA_TEST);

	if (!cursorItem.isNull()) {
		// The cursor item must render in front of every slot regardless of
		// whatever Z offset the slot backgrounds/icons above used - without
		// this it only depth-tests as "on top" while over empty background,
		// and gets hidden behind any slot it's dragged over.
		glDisable2(GL_DEPTH_TEST);
		Tesselator::instance.beginOverride();
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &cursorItem, (float)(xm - 8), (float)(ym - 8), true);
		Tesselator::instance.endOverrideAndDraw();
		glEnable2(GL_DEPTH_TEST);
	}
}

void ArmorScreen::buttonClicked(Button* button) {
	if (button == &btnClose) {
		minecraft->setScreen(NULL);
		return;
	}
	if (button == &btnCraft) {
		minecraft->setScreen(new WorkbenchScreen(Recipe::SIZE_2X2));
		return;
	}
	if (button == &btnItems) {
		minecraft->screenChooser.setScreen(SCREEN_BLOCKSELECTION);
		return;
	}
	if (button->id >= 0 && button->id <= 3) {
		takeAndClearSlot(button->id);
	}
}

bool ArmorScreen::addItem(const Touch::InventoryPane* forPane, int itemIndex) {
	if (!minecraft->useTouchscreen())
		return false;

	if (itemIndex < 0 || itemIndex >= (int)armorItems.size())
		return false;
	const ItemInstance* instance = armorItems[itemIndex];
	if (!ItemInstance::isArmorItem(instance))
		return false;

	ArmorItem* item = (ArmorItem*) instance->getItem();
	ItemInstance* old = player->getArmor(item->slot);
	ItemInstance oldArmor;

	if (ItemInstance::isArmorItem(old)) {
		oldArmor = *old;
	}

	player->setArmor(item->slot, instance);
	player->inventory->removeItem(instance);
	armorItems[itemIndex] = NULL;

	if (!oldArmor.isNull()) {
		if (!player->inventory->add(&oldArmor))
			player->drop(new ItemInstance(oldArmor), false);
	}

	doRecreatePane = true;
	return true;
}

bool ArmorScreen::isAllowed( int slot ) {
	return true;
}

bool ArmorScreen::renderGameBehind() {
	return false;
}

void ArmorScreen::updateItems() {
	armorItems.clear();
	inventorySlots.clear();

	Inventory* inv = minecraft->player->inventory;
	// Show all personal storage slots (9-44), not just armor items.
	const int endSlot = Mth::Min(inv->getContainerSize(), Inventory::CREATIVE_ITEMS_START);
	for (int i = Inventory::MAX_SELECTION_SIZE; i < endSlot; ++i) {
		inventorySlots.push_back(i);
		armorItems.push_back(inv->getItem(i));
	}
}

// Refresh item pointers live every frame so stale pointers are never rendered.
std::vector<const ItemInstance*> ArmorScreen::getItems( const Touch::InventoryPane* forPane ) {
	Inventory* inv = minecraft->player->inventory;
	const int endSlot = Mth::Min(inv->getContainerSize(), Inventory::CREATIVE_ITEMS_START);
	armorItems.clear();
	for (int i = Inventory::MAX_SELECTION_SIZE; i < endSlot; ++i)
		armorItems.push_back(inv->getItem(i));
	return armorItems;
}

bool ArmorScreen::canMoveToSlot(int slot, const ItemInstance* item) {
	return ItemInstance::isArmorItem(item)
		&& ((ArmorItem*)item)->slot == slot;
}

void ArmorScreen::setupInventoryPane() {
	if (inventoryPane) delete inventoryPane;
	inventoryPane = new Touch::InventoryPane(this, minecraft, inventoryPaneRect, inventoryPaneRect.w, BorderPixels, inventorySlots.size(), ItemSize, (int)BorderPixels);
	inventoryPane->fillMarginX = 0;
	inventoryPane->fillMarginY = 0;
}

void ArmorScreen::drawSlotItemAt( Tesselator& t, int slot, const ItemInstance* item, int x, int y)
{
	float xx = (float)x;
	float yy = (float)y;

	guiSlot->draw(t, xx, yy);

	if (item && !item->isNull()) {
		// Blocks with no gui_blocks.png atlas entry (getAtlasPos() < 0 - true
		// for every tile added after that atlas was authored, e.g. the whole
		// spruce set) fall back to ItemRenderer's real 3D tile-preview render,
		// which pushes its own Z translation (glTranslatef2(...,-8) scaled
		// 10x) independent of guiSlot's NinePatch Z. That puts them deeper
		// than the slot background, so they lose the depth test and render
		// behind it, while ordinary mapped icons (a flat quad at Z 0) don't.
		// Disabling depth test for just this draw makes it always win, which
		// is what a slot's contents should do regardless of which path drew them.
		glDisable2(GL_DEPTH_TEST);
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, item, xx + 2, yy, true);
        glDisable2(GL_TEXTURE_2D);
        ItemRenderer::renderGuiItemDecorations(item, xx + 2, yy + 3);
        glEnable2(GL_TEXTURE_2D);
		glEnable2(GL_DEPTH_TEST);
		//minecraft->gui.renderSlotText(item, xx + 3, yy + 3, true, true);
	} else if (slot >= 0) {
		// Draw armor slot placeholder icon only for armor slots (slot 0-3)
        minecraft->textures->loadAndBindTexture("gui/items.png");
        blit(x + 2, y, 15 * 16, slot * 16, 16, 16, 16, 16);
    }
}

void ArmorScreen::takeAndClearSlot( int slot ) {
	ItemInstance* item = player->getArmor(slot);
	if (!item)
		return;

	int oldSize = minecraft->player->inventory->getNumEmptySlots();

	if (!minecraft->player->inventory->add(item))
		minecraft->player->drop(new ItemInstance(*item), false);

	player->setArmor(slot, NULL);

	int newSize = minecraft->player->inventory->getNumEmptySlots();
	setIfNotSet(doRecreatePane, newSize != oldSize);
}

void ArmorScreen::renderPlayer(float xo, float yo) {
	// Push GL and player state
	glPushMatrix();

	glTranslatef(xo, yo, -200);
	float ss = 45;
	glScalef(-ss, ss, ss);

	glRotatef(180, 0, 0, 1);
	//glDisable(GL_DEPTH_TEST);

	Player* player = (Player*) minecraft->player;
	float oybr = player->yBodyRot;
	float oyr = player->yRot;
	float oxr = player->xRot;

	float t = getTimeS();

	float xd = 10 * Mth::sin(t);//(xo + 51) - xm;
	float yd = 10 * Mth::cos(t * 0.05f);//(yo + 75 - 50) - ym;

	glRotatef(45 + 90, 0, 1, 0);
	glRotatef(-45 - 90, 0, 1, 0);

	const float xtan = Mth::atan(xd / 40.0f) * +20;
	const float ytan = Mth::atan(yd / 40.0f) * -20;

	glRotatef(ytan, 1, 0, 0);

	player->yBodyRot = xtan;
	player->yRot = xtan + xtan;
	player->xRot = ytan;
	glTranslatef(0, player->heightOffset, 0);

	// Push walking anim
	float oldWAP = player->walkAnimPos;
	float oldWAS = player->walkAnimSpeed;
	float oldWASO = player->walkAnimSpeedO;

	// Set new walking anim
	player->walkAnimSpeedO = player->walkAnimSpeed = 0.25f;
	player->walkAnimPos = getTimeS() * player->walkAnimSpeed * SharedConstants::TicksPerSecond;

	EntityRenderDispatcher* rd = EntityRenderDispatcher::getInstance();
	rd->playerRotY = 180;
	rd->render(player, 0, 0, 0, 0, 1);

	// Pop walking anim
	player->walkAnimPos = oldWAP;
	player->walkAnimSpeed = oldWAS;
	player->walkAnimSpeedO = oldWASO;

	//glEnable(GL_DEPTH_TEST);
	// Pop GL and player state
	player->yBodyRot = oybr;
	player->yRot = oyr;
	player->xRot = oxr;

	glPopMatrix();
}

bool ArmorScreen::getPaneSlotAt(Touch::InventoryPane* pane, int x, int y, int& outIndex) const {
	if (!pane) return false;
	ScrollingPane::GridItem gi;
	for (int i = 0; i < (int)inventorySlots.size(); ++i) {
		if (!pane->getGridItemFor_slow(i, gi)) continue;
		if (x >= gi.xf && y >= gi.yf && x < gi.xf + ItemSize && y < gi.yf + ItemSize) {
			outIndex = i;
			return true;
		}
	}
	return false;
}

bool ArmorScreen::handleInventorySlotClick(int slot, int buttonNum) {
	Inventory* inv = minecraft->player->inventory;
	ItemInstance* slotItem = inv->getItem(slot);

	if (buttonNum == MouseAction::ACTION_LEFT) {
		if (cursorItem.isNull()) {
			if (!slotItem || slotItem->isNull()) return false;
			cursorItem = *slotItem;
			inv->clearSlot(slot);
		} else if (!slotItem || slotItem->isNull()) {
			inv->setItem(slot, &cursorItem);
			cursorItem = ItemInstance();
		} else if (ItemInstance::isStackable(&cursorItem, slotItem)) {
			const int maxStack = slotItem->getMaxStackSize();
			const int canMove = Mth::Min(cursorItem.count, Mth::Max(0, maxStack - slotItem->count));
			if (canMove > 0) {
				slotItem->count += canMove;
				cursorItem.count -= canMove;
				if (cursorItem.count <= 0) cursorItem = ItemInstance();
				doRecreatePane = true;
				return true;
			}
			// Swap different item types (or full stack)
			ItemInstance tmp = *slotItem;
			inv->setItem(slot, &cursorItem);
			cursorItem = tmp;
		} else {
			ItemInstance tmp = *slotItem;
			inv->setItem(slot, &cursorItem);
			cursorItem = tmp;
		}
		doRecreatePane = true;
		return true;
	}

	if (buttonNum == MouseAction::ACTION_RIGHT) {
		if (cursorItem.isNull()) {
			if (!slotItem || slotItem->isNull()) return false;
			int takeCount = (slotItem->count + 1) / 2;
			cursorItem = *slotItem;
			cursorItem.count = takeCount;
			slotItem->count -= takeCount;
			if (slotItem->count <= 0) inv->clearSlot(slot);
		} else if (!slotItem || slotItem->isNull()) {
			ItemInstance one = cursorItem;
			one.count = 1;
			inv->setItem(slot, &one);
			cursorItem.count -= 1;
			if (cursorItem.count <= 0) cursorItem = ItemInstance();
		} else if (ItemInstance::isStackable(&cursorItem, slotItem) && slotItem->count < slotItem->getMaxStackSize()) {
			slotItem->count += 1;
			cursorItem.count -= 1;
			if (cursorItem.count <= 0) cursorItem = ItemInstance();
		}
		doRecreatePane = true;
		return true;
	}

	return false;
}

bool ArmorScreen::handleArmorSlotClick(int armorSlot, int buttonNum) {
	if (buttonNum != MouseAction::ACTION_LEFT) return false;
	ItemInstance* equipped = player->getArmor(armorSlot);

	if (cursorItem.isNull()) {
		if (!equipped || equipped->isNull()) return false;
		cursorItem = *equipped;
		player->setArmor(armorSlot, NULL);
		doRecreatePane = true;
		return true;
	}

	if (ItemInstance::isArmorItem(&cursorItem)) {
		ArmorItem* ai = (ArmorItem*)cursorItem.getItem();
		if (ai->slot == armorSlot) {
			if (equipped && !equipped->isNull()) {
				ItemInstance old = *equipped;
				player->setArmor(armorSlot, &cursorItem);
				cursorItem = old;
			} else {
				player->setArmor(armorSlot, &cursorItem);
				cursorItem = ItemInstance();
			}
			doRecreatePane = true;
			return true;
		}
	}
	return false;
}

void ArmorScreen::mouseClicked(int x, int y, int buttonNum) {
	if (minecraft->useTouchscreen()) {
		super::mouseClicked(x, y, buttonNum);
		return;
	}

	if (buttonNum != MouseAction::ACTION_LEFT && buttonNum != MouseAction::ACTION_RIGHT) {
		super::mouseClicked(x, y, buttonNum);
		return;
	}

	// Armor slots
	for (int i = 0; i < NUM_ARMORBUTTONS; ++i) {
		Button* b = armorButtons[i];
		if (x >= b->x && y >= b->y && x < b->x + b->width && y < b->y + b->height) {
			handleArmorSlotClick(i, buttonNum);
			return;
		}
	}

	// Hotbar row
	if (x >= hotbarRect.x && y >= hotbarRect.y && y < hotbarRect.y + hotbarRect.h) {
		int col = (x - hotbarRect.x) / ItemSize;
		if (col >= 0 && col < NUM_HOTBAR) {
			handleHotbarSlotClick(col, buttonNum);
			return;
		}
	}

	// Inventory pane
	int index = -1;
	if (getPaneSlotAt(inventoryPane, x, y, index)) {
		if (index >= 0 && index < (int)inventorySlots.size())
			handleInventorySlotClick(inventorySlots[index], buttonNum);
		return;
	}

	if (buttonNum == MouseAction::ACTION_LEFT)
		super::mouseClicked(x, y, buttonNum);
}

void ArmorScreen::mouseReleased(int x, int y, int buttonNum) {
	if (minecraft->useTouchscreen()) {
		super::mouseReleased(x, y, buttonNum);
		return;
	}
	if (buttonNum == MouseAction::ACTION_LEFT)
		super::mouseReleased(x, y, buttonNum);
}

bool ArmorScreen::handleHotbarSlotClick(int h, int buttonNum) {
	Inventory* inv = minecraft->player->inventory;
	ItemInstance* hotbarItem = inv->getLinked(h);

	if (buttonNum == MouseAction::ACTION_LEFT) {
		if (cursorItem.isNull()) {
			if (!hotbarItem || hotbarItem->isNull()) return false;
			cursorItem = *hotbarItem;
			inv->clearSlot(h);
			doRecreatePane = true;
			return true;
		}
		// Cursor has item — check if hotbar slot is occupied
		int s = inv->linkedSlots[h].inventorySlot;
		bool occupied = (s >= Inventory::MAX_SELECTION_SIZE && s < inv->numTotalSlots
		                 && inv->getItem(s) && !inv->getItem(s)->isNull());
		if (occupied) {
			ItemInstance* slotItem = inv->getItem(s);
			if (ItemInstance::isStackable(&cursorItem, slotItem)) {
				int canMove = Mth::Min(cursorItem.count, slotItem->getMaxStackSize() - slotItem->count);
				if (canMove > 0) {
					slotItem->count += canMove;
					cursorItem.count -= canMove;
					if (cursorItem.count <= 0) cursorItem = ItemInstance();
					doRecreatePane = true;
					return true;
				}
			}
			// Swap
			ItemInstance old = *slotItem;
			inv->setItem(s, &cursorItem);
			cursorItem = old;
		} else {
			// Place into empty hotbar slot: find a free storage slot
			int freeSlot = -1;
			for (int i = Inventory::MAX_SELECTION_SIZE; i < Inventory::CREATIVE_ITEMS_START; ++i) {
				ItemInstance* item = inv->getItem(i);
				if (!item || item->isNull()) { freeSlot = i; break; }
			}
			if (freeSlot < 0) return false;
			inv->setItem(freeSlot, &cursorItem);
			inv->linkSlot(h, freeSlot, false);
			cursorItem = ItemInstance();
		}
		doRecreatePane = true;
		return true;
	}

	if (buttonNum == MouseAction::ACTION_RIGHT) {
		if (cursorItem.isNull()) {
			if (!hotbarItem || hotbarItem->isNull()) return false;
			int takeCount = (hotbarItem->count + 1) / 2;
			cursorItem = *hotbarItem;
			cursorItem.count = takeCount;
			hotbarItem->count -= takeCount;
			if (hotbarItem->count <= 0) inv->clearSlot(h);
			doRecreatePane = true;
			return true;
		}
		// Place one item into hotbar slot
		int s = inv->linkedSlots[h].inventorySlot;
		bool occupied = (s >= Inventory::MAX_SELECTION_SIZE && s < inv->numTotalSlots
		                 && inv->getItem(s) && !inv->getItem(s)->isNull());
		if (occupied) {
			ItemInstance* slotItem = inv->getItem(s);
			if (ItemInstance::isStackable(&cursorItem, slotItem)
			    && slotItem->count < slotItem->getMaxStackSize()) {
				slotItem->count += 1;
				cursorItem.count -= 1;
				if (cursorItem.count <= 0) cursorItem = ItemInstance();
				doRecreatePane = true;
				return true;
			}
		} else {
			int freeSlot = -1;
			for (int i = Inventory::MAX_SELECTION_SIZE; i < Inventory::CREATIVE_ITEMS_START; ++i) {
				ItemInstance* item = inv->getItem(i);
				if (!item || item->isNull()) { freeSlot = i; break; }
			}
			if (freeSlot < 0) return false;
			ItemInstance one = cursorItem;
			one.count = 1;
			inv->setItem(freeSlot, &one);
			inv->linkSlot(h, freeSlot, false);
			cursorItem.count -= 1;
			if (cursorItem.count <= 0) cursorItem = ItemInstance();
			doRecreatePane = true;
			return true;
		}
	}
	return false;
}

void ArmorScreen::keyPressed(int eventKey) {
	if (eventKey == Keyboard::KEY_ESCAPE || eventKey == Keyboard::KEY_E)
		minecraft->setScreen(NULL);
}

void ArmorScreen::removed() {
	if (!cursorItem.isNull()) {
		if (!minecraft->player->inventory->add(&cursorItem))
			minecraft->player->drop(new ItemInstance(cursorItem), false);
		cursorItem = ItemInstance();
	}
	super::removed();
}
