#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipe_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipe_H__

//package net.minecraft.world.item.crafting;

//#include "../../inventory/CraftingContainer.h"
#include "../ItemInstance.h"
#include <map>
#include <vector>

class CraftingContainer;
class RecipeGroup;

class ItemPack {
public:
	typedef std::map<int, int> Map;
	//typedef std::vector<std::pair<int, int> > TypeCountList;

	void add(int id, int count = 1);
	int  getCount(int id) const;
	void print() const;

	int getMaxMultipliesOf(ItemPack& v) const;
	std::vector<ItemInstance> getItemInstances() const;

	static int getIdForItemInstance(const ItemInstance* ii);
	static int getIdForItemInstanceAnyAux( const ItemInstance* ii );
	static ItemInstance getItemInstanceForId(int id);
private:
	Map items;
};


// One kind of ingredient in a recipe: <count> items, taken from any of
// <options> (options[0] is the preferred one and is consumed first). Plain
// ingredients have a single option; group ingredients have several.
struct RecipeIngredient {
	RecipeIngredient() : group(NULL), count(0) {}

	std::vector<ItemInstance> options;
	const RecipeGroup* group; // NULL for a plain ingredient
	int count;
};

class Recipe
{
public:
	static const int SIZE_2X2;// = 0;
	static const int SIZE_3X3;// = 1;
	static const int ANY_AUX_VALUE = -1;

	virtual ~Recipe() {}
    virtual bool matches(CraftingContainer* craftSlots) = 0;
	virtual const ItemPack& getItemPack() { return myItems; }
	virtual int getMaxCraftCount(ItemPack& fromItems) = 0;

    virtual int size() = 0;

    virtual ItemInstance assemble(CraftingContainer* craftSlots) = 0;
    virtual ItemInstance getResultItem() const = 0;

	static bool isAnyAuxValue(const ItemInstance* ii) {
		return isAnyAuxValue(ii->id);
	}

	virtual int getCraftingSize() = 0;

	// Variants: a recipe using a RecipeGroup has one variant per member of
	// its first group (e.g. one per wood species). The variant chooses the
	// preferred ingredient, and the output for recipes with a group result.
	// Recipes without groups have a single variant.
	virtual int getNumVariants() const { return 1; }
	virtual ItemInstance getVariantResult(int variant) const { return getResultItem(); }
	// True if the variant's group ingredient must be that exact member
	// (group results), false if other members may fill in for it.
	virtual bool isVariantExclusive() const { return false; }
	virtual std::vector<RecipeIngredient> getIngredients(int variant);
private:
	static bool isAnyAuxValue(int id);
protected:
	ItemPack myItems;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipe_H__*/
