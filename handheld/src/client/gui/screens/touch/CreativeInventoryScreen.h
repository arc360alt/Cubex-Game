#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__CreativeInventoryScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__CreativeInventoryScreen_H__

#include <vector>
#include "../../Screen.h"
#include "../../components/ScrollingPane.h"
#include "../../components/Button.h"
#include "../../components/ImageButton.h"
#include "../../../player/input/touchscreen/TouchAreaModel.h"

class ItemInstance;
class Minecraft;

namespace Touch {

// Standalone creative-mode item picker. Unlike Touch::IngameBlockSelectionScreen
// (used for the survival hotbar "more items" list), this screen owns its own
// item catalog (built once in buildCatalog()) instead of reading it out of the
// player's live Inventory slots, and it never enters the Tesselator's
// "override batching" render mode, so it can't hit the red-square fallback
// InventoryPane::renderBatch falls back to (see ItemRenderer::renderGuiItem).
class CreativeInventoryScreen: public Screen
{
	typedef Screen super;
public:
	CreativeInventoryScreen();
	virtual ~CreativeInventoryScreen();

	virtual void init();
	virtual void setupPositions();
	virtual void removed();

	void tick();
	void render(int xm, int ym, float a);

	bool hasClippingArea(IntRectangle& out);

	void onCatalogItemClicked(int catalogIndex);
	int getCatalogSize() const { return (int)_catalog.size(); }
	const ItemInstance* getCatalogItem(int index) const;

	void buttonClicked(Button* button);
protected:
	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);

private:
	void buildCatalog();
	void freeCatalog();

	class CatalogPane: public ScrollingPane
	{
		typedef ScrollingPane super;
	public:
		CatalogPane(CreativeInventoryScreen* owner, Minecraft* mc, const IntRectangle& rect, int columns, int numItems, float screenScale, int itemBorderPixels);
		~CatalogPane();

		void renderBatch(std::vector<GridItem>& items, float alpha);
		bool onSelect(int gridId, bool selected);

		IArea* clickArea;
	private:
		CreativeInventoryScreen* owner;
		Minecraft* mc;
		int itemBorderPixels;
		int lastClickedIndex;
		int lastClickedTicks;
	};

	std::vector<ItemInstance*> _catalog;
	CatalogPane* _pane;

	THeader bHeader;
	ImageButton bDone;

	IntRectangle clippingArea;
	IntRectangle _rect;
	int _columns;
};

}

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__CreativeInventoryScreen_H__*/
