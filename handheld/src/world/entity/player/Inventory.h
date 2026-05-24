#ifndef NET_MINECRAFT_WORLD_ENTITY_PLAYER__JInventory_H__
#define NET_MINECRAFT_WORLD_ENTITY_PLAYER__JInventory_H__

//package net.minecraft.world.entity.player;

#include "../../inventory/FillingContainer.h"
#include <vector>

class Tile;
class Entity;
class Player;
class ListTag;
class CompoundTag;


class Inventory: public FillingContainer {
	public:
		// Copy all items from another inventory
		void copyFrom(const Inventory& other);
	typedef FillingContainer super;
public:
	// Returns true if this inventory is in creative mode
	bool isCreativeMode() const { return _isCreative; }

	// Fills with creative defaults if empty and in creative mode
	void fillCreativeDefaultsIfEmpty() {
		if (_isCreative && isEmpty()) setupDefault();
	}
	void load(ListTag* inventoryList); // Add declaration for load
	static const int INVENTORY_SIZE_DEMO = 27;
	static const int MAX_SELECTION_SIZE = 9; // Including "More..." right now
	static const int POP_TIME_DURATION = 5;
	// Creative mode: slots [MAX_SELECTION_SIZE, CREATIVE_ITEMS_START) are personal storage;
	// creative item picker starts at CREATIVE_ITEMS_START.
	static const int CREATIVE_PERSONAL_SLOTS = 36;
	static const int CREATIVE_ITEMS_START = MAX_SELECTION_SIZE + CREATIVE_PERSONAL_SLOTS; // 45

	// If skipDefault is true, setupDefault() will NOT be called in constructor
	Inventory(Player* player, bool creativeMode, bool skipDefault = false);
	~Inventory();

	void clearInventoryWithDefault();
	//
	// Selection slots
	//
	void selectSlot(int slot);
	ItemInstance* getSelected();

	static int getSelectionSize();
	// Special for this "selection based" inventory
	bool moveToSelectionSlot(int selectionSlot, int inventorySlot, bool propagate);
	bool moveToSelectedSlot(int inventorySlot, bool propagate);
	bool moveToEmptySelectionSlot(int inventorySlot);

	bool removeItem(const ItemInstance* samePtr);
	void setCreativeModeEnabled(bool enabled);

	void doDrop(ItemInstance* item, bool randomly);
	bool stillValid(Player* player);
	bool add(ItemInstance* item);

	int getAttackDamage(Entity* entity);
	float getDestroySpeed(Tile* tile);
	bool canDestroy(Tile* tile);
private:
	void setupDefault();
	bool isEmpty() const {
		for (size_t i = 0; i < items.size(); ++i) {
			if (items[i] != NULL) return false;
		}
		return true;
	}
public:
	//ItemList armor;

	int selected;
	Player* player;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PLAYER__JInventory_H__*/
