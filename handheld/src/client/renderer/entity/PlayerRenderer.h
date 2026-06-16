#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__

#include "HumanoidMobRenderer.h"

class PlayerRenderer : public HumanoidMobRenderer
{
	typedef HumanoidMobRenderer super;
public:
	PlayerRenderer(HumanoidModel* humanoidModel, float shadow);
	~PlayerRenderer();

	virtual int prepareArmor(Mob* mob, int layer, float a);

	virtual void setupPosition(Entity* mob, float x, float y, float z);
	virtual void setupRotations(Entity* mob, float bob, float bodyRot, float a);
	virtual void render(Entity* mob, float x, float y, float z, float rot, float a);

	virtual void renderName(Mob* mob, float x, float y, float z);
	virtual void onGraphicsReset();
private:
	HumanoidModel* playerModel;   // same pointer held by MobRenderer, cast for direct access
	HumanoidModel* armorParts1;
	HumanoidModel* armorParts2;
	std::string    lastSkinName;  // cached so we only query texture dimensions on change
};


#endif /* NET_MINECRAFT_CLIENT_RENDERER_ENTITY__PlayerRenderer_H__ */