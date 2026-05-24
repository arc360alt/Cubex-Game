#ifndef _FURNACESCREEN_H__
#define _FURNACESCREEN_H__

#include "BaseContainerScreen.h"

#include "../components/InventoryPane.h"
#include "../components/Button.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;
class Tesselator;

class FurnaceScreen:	public BaseContainerScreen,
						public Touch::IInventoryPaneCallback
{
	typedef BaseContainerScreen super;
	typedef std::vector<CItem*> ItemList;
public:
    FurnaceScreen(Player* player, FurnaceTileEntity* furnace);
	~FurnaceScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void buttonClicked(Button* button);
	void mouseClicked(int x, int y, int buttonNum);
	void mouseReleased(int x, int y, int buttonNum);

	// IInventoryPaneCallback
	bool addItem(const Touch::InventoryPane* pane, int itemId);
	bool isAllowed( int slot );
	std::vector<const ItemInstance*> getItems( const Touch::InventoryPane* forPane );
private:
	//void addItem(Recipe* recipe);
	void recheckRecipes();

	void clearItems();
	void updateResult(const ItemInstance* item);
	void setupInventoryPane();
	void updateItems();

	void drawSlotItemAt(Tesselator& t, const ItemInstance* item, int x, int y, bool selected);
	ItemInstance moveOver(const ItemInstance* item, int maxCount);
	void takeAndClearSlot( int slot );
	bool handleAddItem( int slot, const ItemInstance* item );
	bool getPaneSlotAt(Touch::InventoryPane* pane, const std::vector<const ItemInstance*>& items, int x, int y, int& outIndex) const;
	int getFurnaceSlotAt(int x, int y) const;
	bool canPlaceInFurnaceSlot(int slot, const ItemInstance& item) const;
	bool handleInventorySlotClick(FillingContainer* container, int slot, int buttonNum);
	bool handleFurnaceSlotClick(int slot, int buttonNum);
	void sendFurnaceSlotUpdate(int slot);
	void handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a);
	bool canMoveToFurnace(int inventorySlot, const ItemInstance* item);
	ItemList _items;

	std::string currentItemDesc;
	ItemInstance burnResult;
	float descWidth;
	ImageButton btnClose;
	BlankButton btnIngredient;
	BlankButton btnFuel;
	BlankButton btnResult;
	Touch::THeader bHeader;

	Touch::InventoryPane* inventoryPane;
	IntRectangle inventoryPaneRect;
	
	ItemList listFuel;
	ItemList listIngredient;
	std::vector<int> inventorySlots;
	std::vector<const ItemInstance*> inventoryItems;
	bool doRecreatePane;
	ItemInstance cursorItem;

	int selectedSlot;
	int lastBurnTypeId;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiSlotMarked;
	NinePatchLayer* guiSlotMarker;
	NinePatchLayer* guiPaneFrame;
	Player* player;
	FurnaceTileEntity* furnace;
};

#endif /*_FURNACESCREEN_H__*/
