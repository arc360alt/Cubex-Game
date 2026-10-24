#include "Recipe.h"
#include "../../../util/Mth.h"
#include "../../level/tile/Tile.h"

void ItemPack::add( int id, int count /* = 1 */ )
{
	Map::iterator it = items.find(id);
	if (it == items.end()) {
		items.insert(std::make_pair(id, count));
	} else {
		it->second += count;
	}
}

void ItemPack::print() const
{
	Map::const_iterator it = items.begin();
	while (it != items.end()) {
		ItemInstance item = getItemInstanceForId(it->first);
		//LOGI("> %d - %d, %d\n", it->first, item.id, item.getAuxValue());
		++it;
	}
}

int ItemPack::getMaxMultipliesOf( ItemPack& v ) const
{
	if (v.items.empty())
		return 0;
	int minCount = 99;

	Map::iterator it = v.items.begin();
	while (it != v.items.end()) {
		if (it->first <= 0) {
			++it;
			continue;
		}

		Map::const_iterator jt = items.find(it->first);
		if (jt == items.end()) {
			//LOGI("shotto: %d (%s) wasn't found!\n", it->first, getItemInstanceForId(it->first).toString().c_str());
			return 0;
		}
		if (it->second == 0) {
			//LOGE("getMaxMultipliesOf: Can't have count 0 of item: %d\n", it->first);
			return 0;
		}

		int count = jt->second / it->second;
		if (count == 0) return 0;
		minCount = Mth::Min(minCount, count);
		++it;
	}
	return minCount;
}

std::vector<ItemInstance> ItemPack::getItemInstances() const
{
	std::vector<ItemInstance> out;
	Map::const_iterator it = items.begin();
	while (it != items.end()) {
		int id = it->first;
		int count = it->second;
		
		ItemInstance item = getItemInstanceForId(id);
		item.count = count;
		out.push_back(item);
		++it;
	}
	return out;
}

int ItemPack::getIdForItemInstance( const ItemInstance* ii )
{
	bool anyAuxValue = Recipe::isAnyAuxValue(ii);
	return ii->id * 512 + (anyAuxValue? -1/*Recipes::ANY_AUX_VALUE*/ : ii->getAuxValue());
}
int ItemPack::getIdForItemInstanceAnyAux( const ItemInstance* ii )
{
	return ii->id * 512 - 1;
}

ItemInstance ItemPack::getItemInstanceForId( int id )
{
	id += 256;
	return ItemInstance(id / 512, 1, (id & 511) - 256);
}

int ItemPack::getCount( int id ) const
{
	Map::const_iterator it = items.find(id);
	if (it == items.end()) return 0;
	return it->second;
}


std::vector<RecipeIngredient> Recipe::getIngredients( int variant )
{
	std::vector<RecipeIngredient> out;
	std::vector<ItemInstance> items = getItemPack().getItemInstances();
	for (unsigned int i = 0; i < items.size(); ++i) {
		RecipeIngredient ing;
		ing.count = items[i].count;
		items[i].count = 1;
		ing.options.push_back(items[i]);
		out.push_back(ing);
	}
	return out;
}

/*static*/
const int Recipe::SIZE_2X2 = 0;
const int Recipe::SIZE_3X3 = 1;

bool Recipe::isAnyAuxValue( int id )
{
	bool isTile = id < 256;
	if (!isTile) return false;
	if (id == Tile::cloth->id
	 || id == Tile::stoneSlabHalf->id
	 || id == Tile::sandStone->id
	 // treeTrunk's aux value is the wood species (oak/spruce/birch) - it
	 // must matter here the same way it matters in ShapedRecipe::matches(),
	 // otherwise ItemPack (used for the crafting screen's ingredient icon
	 // and have/need counts) pools every species into one "any aux" bucket:
	 // the requirement icon renders with aux 0 (always looks like oak) and
	 // any species in the player's inventory satisfies it, regardless of
	 // what the recipe actually asked for.
	 || id == Tile::treeTrunk->id)
		return false;
	return true;
}
