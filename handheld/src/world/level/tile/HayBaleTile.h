#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__HayBaleTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__HayBaleTile_H__

#include "Tile.h"
#include "../material/Material.h"
#include "../../Facing.h"

class HayBaleTile : public Tile
{
    typedef Tile super;
public:
    HayBaleTile(int id, int texTop, int texSide)
    :   super(id, texTop, Material::dirt),
        texTop(texTop),
        texSide(texSide)
    {}

    int getTexture(int face) override {
        if (face == Facing::UP || face == Facing::DOWN) return texTop;
        return texSide;
    }

    int getTexture(int face, int data) override {
        return getTexture(face);
    }

private:
    int texTop;
    int texSide;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__HayBaleTile_H__*/
