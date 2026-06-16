#ifndef NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__

//package net.minecraft.client.model;

#include "Model.h"
#include "geom/ModelPart.h"
class ItemInstance;
class HumanoidModel: public Model
{
public:
    HumanoidModel(float g = 0, float yOffset = 0);

	void setupAnim(float time, float r, float bob, float yRot, float xRot, float scale);

	void render(HumanoidModel* model, float scale);
    void render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale);
	void renderHorrible(float time, float r, float bob, float yRot, float xRot, float scale);
	void onGraphicsReset();
	void reinitArms(bool isAlex);
	void checkSkinType();

	// Switch between 64x32 (pre-1.8) and 64x64 (1.8+) skin formats.
	// Rebuilds UV-dependent geometry and shows/hides overlay parts accordingly.
	void setSkinIs64(bool is64);

	float g_val;
	float yOffset_val;
	int lastSkinType; // 0 = Steve, 1 = Alex
	bool skinIs64;    // true when the current skin texture is 64x64

	// Base layer parts (used in both skin formats).
	ModelPart head, body, arm0, arm1, leg0, leg1;
	// Head overlay (hat): exists in both 64x32 and 64x64.
	ModelPart hat;
	// Body/arm/leg overlays: only visible in 64x64 skins.
	ModelPart bodyOverlay, arm0Overlay, arm1Overlay, leg0Overlay, leg1Overlay;

	bool holdingLeftHand;
	bool holdingRightHand;
	bool sneaking;
	bool bowAndArrow;
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__HumanoidModel_H__*/
