#include "Recipes.h"
#include "ShapedRecipe.h"
#include "ShapelessRecipe.h"

#include "ClothDyeRecipes.h"
#include "ToolRecipes.h"
#include "WeaponRecipes.h"
#include "StructureRecipes.h"
#include "FoodRecipes.h"
#include "FurnaceRecipes.h"
#include "ArmorRecipes.h"
#include "OreRecipes.h"
#include "../CoalItem.h"
#include "../../level/tile/StoneSlabTile.h"
#include "../../level/tile/TreeTile.h"

/*static*/
Recipes* Recipes::instance = NULL;

RecipeGroup* RecipeGroup::logs = NULL;
RecipeGroup* RecipeGroup::planks = NULL;
RecipeGroup* RecipeGroup::woodStairs = NULL;
RecipeGroup* RecipeGroup::woodSlabs = NULL;
RecipeGroup* RecipeGroup::woodDoors = NULL;
RecipeGroup* RecipeGroup::woodTrapdoors = NULL;

void RecipeGroup::initGroups()
{
	// Wood groups: members in species order oak, spruce, birch, so that
	// they can be paired as ingredient/result (see RecipeGroup.h).
	logs = (new RecipeGroup("logs"))
		->add(ItemInstance(Tile::treeTrunk, 1, TreeTile::NORMAL_TRUNK))
		->add(ItemInstance(Tile::treeTrunk, 1, TreeTile::DARK_TRUNK))
		->add(ItemInstance(Tile::treeTrunk, 1, TreeTile::BIRCH_TRUNK));
	planks = (new RecipeGroup("planks"))
		->add(ItemInstance(Tile::wood, 1, Recipe::ANY_AUX_VALUE))
		->add(ItemInstance(Tile::planksSpruce, 1, Recipe::ANY_AUX_VALUE))
		->add(ItemInstance(Tile::planksBirch, 1, Recipe::ANY_AUX_VALUE));
	woodStairs = (new RecipeGroup("woodStairs"))
		->add(ItemInstance(Tile::stairs_wood))
		->add(ItemInstance(Tile::stairs_spruce))
		->add(ItemInstance(Tile::stairs_birch));
	woodSlabs = (new RecipeGroup("woodSlabs"))
		->add(ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::WOOD_SLAB))
		->add(ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SPRUCE_SLAB))
		->add(ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::BIRCH_SLAB));
	woodDoors = (new RecipeGroup("woodDoors"))
		->add(ItemInstance(Item::door_wood))
		->add(ItemInstance(Item::door_spruce))
		->add(ItemInstance(Item::door_birch));
	woodTrapdoors = (new RecipeGroup("woodTrapdoors"))
		->add(ItemInstance(Tile::trapdoor))
		->add(ItemInstance(Tile::trapdoor_spruce))
		->add(ItemInstance(Tile::trapdoor_birch));
}

void RecipeGroup::teardownGroups()
{
	delete logs;       logs = NULL;
	delete planks;     planks = NULL;
	delete woodStairs; woodStairs = NULL;
	delete woodSlabs;  woodSlabs = NULL;
	delete woodDoors;  woodDoors = NULL;
	delete woodTrapdoors; woodTrapdoors = NULL;
}

Recipes::Recipes()
{
	RecipeGroup::initGroups();

	ToolRecipes::addRecipes(this);
	WeaponRecipes::addRecipes(this);
	OreRecipes::addRecipes(this);
	FoodRecipes::addRecipes(this);
	StructureRecipes::addRecipes(this);
	ArmorRecipes::addRecipes(this);
	ClothDyeRecipes::addRecipes(this);

	addShapedRecipe(ItemInstance(Item::paper, 3), //
		"###", //

		definition('#', Item::reeds));

	addShapedRecipe(ItemInstance(Item::book, 1), //
		"#", //
		"#", //
		"#", //

		definition('#', Item::paper));

	addShapedRecipe(ItemInstance(Tile::fence, 2), //
		"###", //
		"###", //

		definition('#', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::netherFence, 6), //
	//	"###", //
	//	"###", //

	//	'#', Tile::netherBrick);

	addShapedRecipe(ItemInstance(Tile::fenceGate, 1), //
		"#W#", //
		"#W#", //

		definition('#', Item::stick, 'W', RecipeGroup::planks));

	//addShapedRecipe(ItemInstance(Tile::recordPlayer, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::wood, 'X', Item::emerald));

	//addShapedRecipe(ItemInstance(Tile::musicBlock, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::wood, 'X', Item::redStone));

	addShapedRecipe(ItemInstance(Tile::bookshelf, 1), //
		"###", //
		"XXX", //
		"###", //

		definition('#', RecipeGroup::planks, 'X', Item::book));

	addShapedRecipe(ItemInstance(Tile::snow, 1), //
		"##", //
		"##", //

		definition('#', Item::snowBall));

	addShapedRecipe(ItemInstance(Tile::clay, 1), //
		"##", //
		"##", //

		definition('#', Item::clay));

	addShapedRecipe(ItemInstance(Tile::redBrick, 1), //
		"##", //
		"##", //

		definition('#', Item::brick));

	addShapedRecipe(ItemInstance(Tile::lightGem, 1), //
		"##", //
		"##", //

		definition('#', Item::yellowDust));

	addShapedRecipe(ItemInstance(Tile::cloth, 1), //
		"##", //
		"##", //

		definition('#', Item::string));

	addShapedRecipe(ItemInstance(Tile::tnt, 1), //
		"X#X", //
		"#X#", //
		"X#X", //

		definition(	'X', Item::sulphur,
					'#', Tile::sand));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::COBBLESTONE_SLAB), //
		"###", //

		definition('#', Tile::stoneBrick));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::STONE_SLAB), //
		"###", //

		definition('#', Tile::rock));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::SAND_SLAB), //
		"###", //
		definition('#', Tile::sandStone));

	addShapedRecipe(RecipeGroup::woodSlabs, 6, //
		Shape("###"), //

		definition('#', RecipeGroup::planks));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::BRICK_SLAB), //
		"###", //

		definition('#', Tile::redBrick));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::SMOOTHBRICK_SLAB), //
		"###", //
		definition('#', Tile::stoneBrickSmooth));

	addShapedRecipe(ItemInstance(Tile::ladder, 2), //
		"# #", //
		"###", //
		"# #", //

		definition('#', Item::stick));

	addShapedRecipe(RecipeGroup::woodDoors, 1, //
		Shape("##", //
			  "##", //
			  "##"), //

		definition('#', RecipeGroup::planks));

	addShapedRecipe(RecipeGroup::woodTrapdoors, 2, //
		Shape("###", //
			  "###"), //

		definition('#', RecipeGroup::planks));

	//addShapedRecipe(ItemInstance(Item::door_iron, 1), //
	//	"##", //
	//	"##", //
	//	"##", //

	//	definition('#', Item::ironIngot));

	addShapedRecipe(ItemInstance(Item::sign, 1), //
		"###", //
		"###", //
		" X ", //

		definition('#', RecipeGroup::planks, 'X', Item::stick));

	//addShapedRecipe(ItemInstance(Item::cake, 1), //
	//	"AAA", //
	//	"BEB", //
	//	"CCC", //

	//	definition(	'A', Item::milk,//
	//				'B', Item::sugar,//
	//				'C', Item::wheat, 'E', Item::egg));

	addShapedRecipe(ItemInstance(Item::sugar, 1), //
		"#", //

		definition('#', Item::reeds));

	// One recipe for every species: the planks match the log used.
	addShapedRecipe(RecipeGroup::planks, 4, //
		Shape("#"), //

		definition('#', RecipeGroup::logs));

	addShapedRecipe(ItemInstance(Item::stick, 4), //
		"#", //
		"#", //

		definition('#', RecipeGroup::planks));

	addShapedRecipe(ItemInstance(Tile::torch, 4), //
		"X", //
		"#", //

		definition(	'X', Item::coal,//
					'#', Item::stick));
	// torch made of charcoal
	addShapedRecipe(ItemInstance(Tile::torch, 4), //
		"X", //
		"#", //

		definition( 'X', ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL),//
					'#', Item::stick));

	addShapedRecipe(ItemInstance(Tile::lantern, 1), //
		"I", //
		"T", //

		definition('I', Item::ironIngot, 'T', Tile::torch));

	addShapedRecipe(ItemInstance(Item::bowl, 4), //
		"# #", //
		" # ", //

		definition('#', RecipeGroup::planks));

	//addShapedRecipe(ItemInstance(Item::glassBottle, 3), //
	//	"# #", //
	//	" # ", //

	//	'#', Tile::glass);

	//addShapedRecipe(ItemInstance(Tile::rail, 16), //
	//	"X X", //
	//	"X#X", //
	//	"X X", //

	//	definition(	'X', Item::ironIngot,//
	//				'#', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::goldenRail, 6), //
	//	"X X", //
	//	"X#X", //
	//	"XRX", //

	//	'X', Item::goldIngot,//
	//	'R', Item::redStone,//
	//	'#', Item::stick);

	//addShapedRecipe(ItemInstance(Tile::detectorRail, 6), //
	//	"X X", //
	//	"X#X", //
	//	"XRX", //

	//	'X', Item::ironIngot,//
	//	'R', Item::redStone,//
	//	'#', Tile::pressurePlate_stone);

	//addShapedRecipe(ItemInstance(Item::minecart, 1), //
	//	"# #", //
	//	"###", //

	//	definition('#', Item::ironIngot));

	//addShapedRecipe(ItemInstance(Item::cauldron, 1), //
	//	"# #", //
	//	"# #", //
	//	"###", //

	//	'#', Item::ironIngot);

	//addShapedRecipe(ItemInstance(Item::brewingStand, 1), //
	//	" B ", //
	//	"###", //

	//	'#', Tile::stoneBrick, 'B', Item::blazeRod);

	//addShapedRecipe(ItemInstance(Tile::litPumpkin, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::pumpkin, 'B', Tile::torch));

	//addShapedRecipe(ItemInstance(Item::minecart_chest, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::chest, 'B', Item::minecart));

	//addShapedRecipe(ItemInstance(Item::minecart_furnace, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::furnace, 'B', Item::minecart));

	//addShapedRecipe(ItemInstance(Item::boat, 1), //
	//	"# #", //
	//	"###", //

	//	definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Item::bucket_empty, 1), //
	//	"# #", //
	//	" # ", //

	//	definition('#', Item::ironIngot));

	addShapedRecipe(ItemInstance(Item::flintAndSteel, 1), //
		"A ", //
		" B", //

		definition('A', Item::ironIngot, 'B', Item::flint));

	addShapedRecipe(ItemInstance(Item::bread, 1), //
		"###", //

		definition('#', Item::wheat));

	addShapedRecipe(RecipeGroup::woodStairs, 4, //
		Shape("#  ", //
			  "## ", //
			  "###"), //

		definition('#', RecipeGroup::planks));

	//addShapedRecipe(ItemInstance(Item::fishingRod, 1), //
	//	"  #", //
	//	" #X", //
	//	"# X", //

	//	definition('#', Item::stick, 'X', Item::string));

	addShapedRecipe(ItemInstance(Tile::stairs_stone, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::stoneBrick));

	addShapedRecipe(ItemInstance(Tile::stairs_brick, 4), //
		"#  ", //
		"## ", //
		"###", //
		definition('#', Tile::redBrick));

	addShapedRecipe(ItemInstance(Tile::stairs_stoneBrickSmooth, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::stoneBrickSmooth));

	addShapedRecipe(ItemInstance(Tile::stairs_netherBricks, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::netherBrick));

	addShapedRecipe(ItemInstance(Item::painting, 1), //
		"###", //
		"#X#", //
		"###", //

		definition('#', Item::stick, 'X', Tile::cloth));

	//addShapedRecipe(ItemInstance(Item::apple_gold, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::goldBlock, 'X', Item::apple));

	//addShapedRecipe(ItemInstance(Tile::lever, 1), //
	//	"X", //
	//	"#", //

	//	definition('#', Tile::stoneBrick, 'X', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::notGate_on, 1), //
	//	"X", //
	//	"#", //

	//	definition('#', Item::stick, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::diode, 1), //
	//	"#X#", //
	//	"III", //

	//	definition('#', Tile::notGate_on, 'X', Item::redStone, 'I', Tile::rock));


	//addShapedRecipe(ItemInstance(Item::clock, 1), //
	//	" # ", //
	//	"#X#", //
	//	" # ", //

	//	definition('#', Item::goldIngot, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::compass, 1), //
	//	" # ", //
	//	"#X#", //
	//	" # ", //

	//	definition('#', Item::ironIngot, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::map, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	'#', Item::paper, 'X', Item::compass);

	//addShapedRecipe(ItemInstance(Tile::button, 1), //
	//	"#", //
	//	"#", //

	//	definition('#', Tile::rock));


	//addShapedRecipe(ItemInstance(Tile::pressurePlate_stone, 1), //
	//	"##", //

	//	definition('#', Tile::rock));

	//addShapedRecipe(ItemInstance(Tile::pressurePlate_wood, 1), //
	//	"##", //

	//	definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Tile::dispenser, 1), //
	//	"###", //
	//	"#X#", //
	//	"#R#", //

	//	definition('#', Tile::stoneBrick, 'X', Item::bow, 'R', Item::redStone));

	//addShapedRecipe(ItemInstance(Tile::pistonBase, 1), //
	//	"TTT", //
	//	"#X#", //
	//	"#R#", //

	//	'#', Tile::stoneBrick, 'X', Item::ironIngot, 'R', Item::redStone, 'T', Tile::wood);

	//addShapedRecipe(ItemInstance(Tile::pistonStickyBase, 1), //
	//	"S", //
	//	"P", //

	//	'S', Item::slimeBall, 'P', Tile::pistonBase);

	addShapedRecipe(ItemInstance(Item::bed, 1), //
		"###", //
		"XXX", //
		definition('#', Tile::cloth, 'X', RecipeGroup::planks));

	//addShapedRecipe(ItemInstance(Tile::enchantTable, 1), //
	//	" B ", //
	//	"D#D", //
	//	"###", //

	//	'#', Tile::obsidian, 'B', Item::book, 'D', Item::emerald);

	//addShapelessRecipe(ItemInstance(Item::eyeOfEnder, 1), //
	//	Item::enderPearl, Item::blazePowder);
	addShapedRecipe(ItemInstance(Tile::netherReactor, 1), //
		"X#X", //
		"X#X", //
		"X#X", //

		definition('#', Item::emerald, 'X', Item::ironIngot));

	LOGI("%d recipes\n", (int)recipes.size());
}

void Recipes::addShapedRecipe( const ItemInstance& result, const RowList& rows, const TypeList& types )
{
	addShapedRecipe(result, NULL, rows, types);
}

void Recipes::addShapedRecipe( const RecipeGroup* resultGroup, int resultCount, const RowList& rows, const TypeList& types )
{
	if (!resultGroup || resultGroup->size() == 0) {
		LOGE("Recipes::addShapedRecipe: empty result group!\n");
		return;
	}
	ItemInstance result = resultGroup->get(0);
	result.count = resultCount;
	if (result.getAuxValue() == Recipe::ANY_AUX_VALUE) result.setAuxValue(0);
	addShapedRecipe(result, resultGroup, rows, types);
}

void Recipes::addShapedRecipe( const ItemInstance& result, const RecipeGroup* resultGroup, const RowList& rows, const TypeList& types )
{
	if (rows.empty()) {
		LOGE("Recipes::addShapedRecipe: adding an empty recipe!\n");
		return;
	}

	std::string map = "";
	int width = rows[0].length();
	int height = rows.size();

	for (unsigned int i = 0; i < rows.size(); ++i)
		map += rows[i];

	typedef std::map<char, ItemInstance> Map;
	typedef std::map<char, const RecipeGroup*> GroupMap;
	Map mappings;
	GroupMap groupMappings;
	for (unsigned int i = 0; i < types.size(); i++) {
		const Type& type = types[i];
		char from = type.c;
		ItemInstance to;

		if (type.group) {
			// The slot holds the first member, so code that only knows about
			// plain ingredients (e.g. ItemPack) still sees something sensible
			to = type.group->get(0);
			groupMappings.insert(std::make_pair(from, type.group));
		} else if (type.item) {
			to = ItemInstance(type.item);
		} else if (type.tile) {
			to = ItemInstance(type.tile, 1, Recipe::ANY_AUX_VALUE);
		} else if (!type.itemInstance.isNull()) {
			to = type.itemInstance;
		}
		
		mappings.insert(std::make_pair(from, to));
	}

	ItemInstance* ids = new ItemInstance[width * height];
	const RecipeGroup** groups = groupMappings.empty()? NULL : new const RecipeGroup*[width * height];

	for (int i = 0; i < width * height; i++) {
		char ch = map[i];
		Map::iterator it = mappings.find(ch);
		if (it != mappings.end())
			ids[i] = it->second;
		if (groups) {
			GroupMap::iterator gt = groupMappings.find(ch);
			groups[i] = (gt != groupMappings.end())? gt->second : NULL;
		}
	}

	if (resultGroup && (groupMappings.empty() || groupMappings.begin()->second->size() != resultGroup->size()))
		LOGE("Recipes::addShapedRecipe: result group %s doesn't pair with the ingredient group!\n", resultGroup->name.c_str());

	// <ids> and <groups> are deleted in ShapedRecipe
	recipes.push_back(new ShapedRecipe(width, height, ids, result, groups, resultGroup));
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const TypeList& types) {
	addShapedRecipe(result, Shape(r0), types);
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const std::string& r1, const TypeList& types) {
	addShapedRecipe(result, Shape(r0, r1), types);
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const std::string& r1, const std::string& r2, const TypeList& types) {
	addShapedRecipe(result, Shape(r0, r1, r2), types);
}

void Recipes::addShapelessRecipe(const ItemInstance& result, const TypeList& types) {

    std::vector<ItemInstance> ingredients;

	for (unsigned int i = 0; i < types.size(); i++) {
		const Type& type = types[i];
		if (type.item) {
			ingredients.push_back( ItemInstance(type.item) );
		} else if (type.tile) {
			ingredients.push_back( ItemInstance(type.tile) );
		} else if (!type.itemInstance.isNull()) {
			ingredients.push_back( type.itemInstance );
		} else {
			LOGE("addShapeLessRecipe: Incorrect shapeless recipe!\n");
		}
    }

    recipes.push_back(new ShapelessRecipe(result, ingredients));
}

RowList Recipes::Shape( const std::string& r0 ) {
	RowList rows;
	rows.push_back(r0);
	return rows;
}

RowList Recipes::Shape( const std::string& r0, const std::string& r1 ) {
	RowList rows;
	rows.push_back(r0);
	rows.push_back(r1);
	return rows;
}

RowList Recipes::Shape( const std::string& r0, const std::string& r1, const std::string& r2 ) {
	RowList rows;
	rows.push_back(r0);
	rows.push_back(r1);
	rows.push_back(r2);
	return rows;
}

Recipe* Recipes::getRecipeFor( const ItemInstance& result )
{
	for (unsigned int i = 0; i < recipes.size(); ++i) {
		Recipe* recipe = recipes[i];
		ItemInstance res = recipe->getResultItem();
		if (result.id != res.id) continue;

		if ( result.count == 0 && result.getAuxValue() == res.getAuxValue()
		||	(result.count == res.count && result.getAuxValue() == res.getAuxValue()))
			return recipe;
	}
	return NULL;
}

Recipes* Recipes::getInstance()
{
	if (!instance)
		instance = new Recipes();

	return instance;
}

void Recipes::teardownRecipes()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
	FurnaceRecipes::teardownFurnaceRecipes();
}

const RecipeList& Recipes::getRecipes()
{
	return recipes;
}

ItemInstance Recipes::getItemFor( CraftingContainer* craftSlots )
{
	int count = 0;
	ItemInstance* first;
	ItemInstance* second;
	for (int i = 0; i < craftSlots->getContainerSize(); i++) {
		ItemInstance* item = craftSlots->getItem(i);
		if (item) {
			if (count == 0) first = item;
			if (count == 1) second = item;
			count++;
		}
	}

	if (count == 2 && first->id == second->id && first->count == 1 && second->count == 1 && Item::items[first->id]->canBeDepleted()) {
		Item* item = Item::items[first->id];
		int remaining1 = item->getMaxDamage() - first->getDamageValue();
		int remaining2 = item->getMaxDamage() - second->getDamageValue();
		int remaining = (remaining1 + remaining2) + item->getMaxDamage() * 10 / 100;
		int resultDamage = item->getMaxDamage() - remaining;
		if (resultDamage < 0) resultDamage = 0;
		return ItemInstance(first->id, 1, resultDamage);
	}

	for (unsigned int i = 0; i < recipes.size(); i++) {
		Recipe* r = recipes[i];
		if (r->matches(craftSlots)) return r->assemble(craftSlots);
	}
	return ItemInstance();
}

Recipes::~Recipes()
{
	for (unsigned int i = 0; i < recipes.size(); ++i)
		delete recipes[i];
	RecipeGroup::teardownGroups();
}
