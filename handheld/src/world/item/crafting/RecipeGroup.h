#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__RecipeGroup_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__RecipeGroup_H__

#include <string>
#include <vector>
#include "../ItemInstance.h"

// A named set of interchangeable ingredients (like item tags in Java
// Edition). A recipe can use a group anywhere it would use a single item,
// e.g. definition('#', RecipeGroup::planks) accepts any wood's planks.
//
// A group can also be a recipe's *result*: the output is the result group's
// member at the same index as the ingredient group member that was used
// (spruce log -> spruce planks). Groups meant to be paired like that must
// list their members in the same order - for wood: oak, spruce, birch.
//
// Members with aux value Recipe::ANY_AUX_VALUE match any aux value.
class RecipeGroup
{
public:
	explicit RecipeGroup(const std::string& name)
	:	name(name)
	{}

	RecipeGroup* add(const ItemInstance& item) {
		members.push_back(item);
		return this;
	}

	int size() const { return (int)members.size(); }
	const ItemInstance& get(int i) const { return members[i]; }

	// Index of the member matching item (id and aux), or -1
	int indexOf(const ItemInstance& item) const {
		for (unsigned int i = 0; i < members.size(); ++i) {
			const ItemInstance& m = members[i];
			if (m.id == item.id && (m.getAuxValue() == -1 || m.getAuxValue() == item.getAuxValue()))
				return (int)i;
		}
		return -1;
	}

	const std::string name;

	// Built-in groups. Created/destroyed together with Recipes, see
	// Recipes.cpp. Add new groups there (and declare them here).
	static RecipeGroup* logs;
	static RecipeGroup* planks;
	static RecipeGroup* woodStairs;
	static RecipeGroup* woodSlabs;
	static RecipeGroup* woodDoors;
	static RecipeGroup* woodTrapdoors;

	static void initGroups();
	static void teardownGroups();
private:
	std::vector<ItemInstance> members;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__RecipeGroup_H__*/
