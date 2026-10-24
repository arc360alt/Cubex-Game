#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__

//package net.minecraft.world.item.crafting;

#include "../../inventory/CraftingContainer.h"
#include "../ItemInstance.h"
#include "Recipe.h"
#include "RecipeGroup.h"

class ShapedRecipe: public Recipe
{
public:
	// slotGroups (optional, width*height, deleted here) marks the slots that
	// take any member of a RecipeGroup; recipeItems holds that group's first
	// member there. With a resultGroup the output is the resultGroup member
	// matching the group member used (count taken from result).
    ShapedRecipe(int width, int height, ItemInstance* recipeItems, const ItemInstance& result,
				 const RecipeGroup** slotGroups = NULL, const RecipeGroup* resultGroup = NULL)
	:	width(width),
		height(height),
		recipeItems(recipeItems),
		slotGroups(slotGroups),
		resultId(result.id),
		result(result),
		resultGroup(resultGroup),
		variantGroup(NULL),
		matchedVariant(0)
		//@todo: move slots to upper left
	{
		for (int i = 0; i < width * height; ++i) {
			if (slotGroups && slotGroups[i] && !variantGroup)
				variantGroup = slotGroups[i];
			if (!recipeItems[i].isNull())
				myItems.add( ItemPack::getIdForItemInstance(&recipeItems[i]));
		}
		if (!variantGroup)
			this->resultGroup = NULL;
    }

	~ShapedRecipe() {
		delete[] recipeItems;
		delete[] slotGroups;
	}

	int getMaxCraftCount(ItemPack& fromItems) {

		int count = fromItems.getMaxMultipliesOf(myItems);
		return count;


		//return (int)(Mth::random() * Mth::random() * 5);
	}

    ItemInstance getResultItem() const {
        return result;
    }

	int getNumVariants() const {
		return variantGroup? variantGroup->size() : 1;
	}

	ItemInstance getVariantResult(int variant) const {
		if (!resultGroup || variant < 0 || variant >= resultGroup->size())
			return result;
		ItemInstance out = resultGroup->get(variant);
		out.count = result.count;
		if (out.getAuxValue() == Recipe::ANY_AUX_VALUE) out.setAuxValue(0);
		return out;
	}

	bool isVariantExclusive() const {
		return resultGroup != NULL;
	}

	std::vector<RecipeIngredient> getIngredients(int variant) {
		std::vector<RecipeIngredient> out;
		for (int i = 0; i < width * height; ++i) {
			const RecipeGroup* group = slotGroups? slotGroups[i] : NULL;
			if (!group && recipeItems[i].isNull())
				continue;

			// Same ingredient as an earlier slot?
			bool found = false;
			for (unsigned int j = 0; j < out.size() && !found; ++j) {
				RecipeIngredient& ing = out[j];
				if (group? (ing.group == group)
						 : (!ing.group && ing.options[0].id == recipeItems[i].id
									   && ing.options[0].getAuxValue() == recipeItems[i].getAuxValue())) {
					ing.count++;
					found = true;
				}
			}
			if (found) continue;

			RecipeIngredient ing;
			ing.count = 1;
			ing.group = group;
			if (!group) {
				ing.options.push_back(recipeItems[i]);
			} else if (group == variantGroup && variant >= 0 && variant < group->size()) {
				ing.options.push_back(group->get(variant));
				if (!isVariantExclusive()) {
					for (int m = 0; m < group->size(); ++m)
						if (m != variant) ing.options.push_back(group->get(m));
				}
			} else {
				for (int m = 0; m < group->size(); ++m)
					ing.options.push_back(group->get(m));
			}
			for (unsigned int o = 0; o < ing.options.size(); ++o)
				ing.options[o].count = 1;
			out.push_back(ing);
		}
		return out;
	}

    bool matches(CraftingContainer* craftSlots) {
        for (int xOffs = 0; xOffs <= (3 - width); xOffs++) {
            for (int yOffs = 0; yOffs <= (3 - height); yOffs++) {
                if (matches(craftSlots, xOffs, yOffs, true)) return true;
                if (matches(craftSlots, xOffs, yOffs, false)) return true;
            }
        }
        return false;
    }

	ItemInstance assemble(CraftingContainer* craftSlots) {
		// matchedVariant was set by the matches() call that picked this recipe
		ItemInstance out = getVariantResult(matchedVariant);
		return ItemInstance(out.id, out.count, out.getAuxValue());
	}

	int size() {
		return width * height;
	}

	virtual std::vector<ItemInstance> getItems() {
		std::vector<ItemInstance> out;
		return out;
	}

private:
    bool matches(CraftingContainer* craftSlots, int xOffs, int yOffs, bool xFlip) {
		int variant = -1;
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                int xs = x - xOffs;
                int ys = y - yOffs;
                ItemInstance expected;
				const RecipeGroup* group = NULL;
                if (xs >= 0 && ys >= 0 && xs < width && ys < height) {
					int index = xFlip? (width - xs - 1) + ys * width : xs + ys * width;
					expected = recipeItems[index];
					if (slotGroups) group = slotGroups[index];
                }
                ItemInstance* item = craftSlots->getItem(x, y);
                if (!item && expected.isNull())
                    continue;
                if ((item == NULL) ^ expected.isNull())
                    return false;
				if (group) {
					int member = group->indexOf(*item);
					if (member < 0)
						return false;
					if (group == variantGroup) {
						if (variant < 0) variant = member;
						else if (isVariantExclusive() && member != variant) return false;
					}
					continue;
				}
                if (expected.id != item->id)
                    return false;
                if (expected.getAuxValue() != Recipe::ANY_AUX_VALUE && expected.getAuxValue() != item->getAuxValue())
                    return false;
            }
        }
		matchedVariant = (variant < 0)? 0 : variant;
        return true;
    }

	int getCraftingSize() {
		return (width <= 2 && height <= 2)? SIZE_2X2 : SIZE_3X3;
	}


public:
	const int resultId;
private:
	int width, height;
	ItemInstance* recipeItems;
	const RecipeGroup** slotGroups;
	ItemInstance result;
	const RecipeGroup* resultGroup;
	const RecipeGroup* variantGroup; // first group ingredient, decides the variant
	int matchedVariant;
	//ItemPack myItems;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__*/
