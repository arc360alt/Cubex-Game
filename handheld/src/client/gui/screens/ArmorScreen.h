#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__

#include "BaseContainerScreen.h"

#include "../components/InventoryPane.h"
#include "../components/Button.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;
class Tesselator;

class ArmorScreen:	public Screen,
					public Touch::IInventoryPaneCallback
{
	typedef Screen super;
	typedef std::vector<CItem*> ItemList;

	static const int NUM_ARMORBUTTONS = 4;
	static const int NUM_HOTBAR = 9;
public:
    ArmorScreen();
	~ArmorScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void buttonClicked(Button* button);
	void keyPressed(int eventKey);

	// IInventoryPaneCallback
	bool addItem(const Touch::InventoryPane* pane, int itemId);
	bool isAllowed( int slot );
	std::vector<const ItemInstance*> getItems( const Touch::InventoryPane* forPane );
private:
	void renderPlayer(float xo, float yo);

	void setupInventoryPane();
	void updateItems();

	void drawSlotItemAt(Tesselator& t, int slot, const ItemInstance* item, int x, int y);
	void takeAndClearSlot( int slot );
	void handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a);
	bool canMoveToSlot(int slot, const ItemInstance* item);

	bool getPaneSlotAt(Touch::InventoryPane* pane, int x, int y, int& outIndex) const;
	bool handleInventorySlotClick(int slot, int buttonNum);
	bool handleArmorSlotClick(int armorSlot, int buttonNum);
	bool handleHotbarSlotClick(int h, int buttonNum);
	void mouseClicked(int x, int y, int buttonNum);
	void mouseReleased(int x, int y, int buttonNum);
	void removed();

	std::vector<int> inventorySlots;
	ItemInstance cursorItem;

	std::string currentItemDesc;
	float descWidth;
	ImageButton btnClose;
	Touch::TButton btnCraft;

	BlankButton btnArmor0;
	BlankButton btnArmor1;
	BlankButton btnArmor2;
	BlankButton btnArmor3;
	BlankButton* armorButtons[4];

	Touch::THeader bHeader;

	Touch::InventoryPane* inventoryPane;
	IntRectangle inventoryPaneRect;
	IntRectangle hotbarRect;
	IntRectangle guiPlayerBgRect;

	std::vector<const ItemInstance*> armorItems;
	bool doRecreatePane;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiPaneFrame;
	NinePatchLayer* guiPlayerBg;
	Player* player;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ArmorScreen_H__*/
