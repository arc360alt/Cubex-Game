#include "PlayerRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/level/Level.h"
#include "../../../world/item/ArmorItem.h"
#include "../Textures.h"
#include "../TextureData.h"
#include "../../model/CapeModel.h"
#include "../gles.h"

static const std::string armorFilenames[10] = {
	"armor/cloth_1.png",	"armor/cloth_2.png",
	"armor/chain_1.png",	"armor/chain_2.png",
	"armor/iron_1.png",		"armor/iron_2.png",
	"armor/diamond_1.png",	"armor/diamond_2.png",
	"armor/gold_1.png",		"armor/gold_2.png",
};

PlayerRenderer::PlayerRenderer( HumanoidModel* humanoidModel, float shadow )
:	super(humanoidModel, shadow),
	playerModel(humanoidModel),
	armorParts1(new HumanoidModel(1.0f)),
	armorParts2(new HumanoidModel(0.5f)),
	capeModel(new CapeModel())
{
}

PlayerRenderer::~PlayerRenderer() {
	delete armorParts1;
	delete armorParts2;
	delete capeModel;
}

void PlayerRenderer::render(Entity* e, float x, float y, float z, float rot, float a) {
	// Detect whether the player's skin is 64x64 (1.8+) or 64x32 (pre-1.8)
	// and configure the model accordingly.  Cache by skin name so we only
	// query texture metadata when the skin actually changes.
	Mob* mob = (Mob*)e;
	const std::string& texName = mob->getTexture();
	if (texName != lastSkinName) {
		lastSkinName = texName;
		TextureId id = entityRenderDispatcher->textures->loadTexture(texName);
		const TextureData* td = entityRenderDispatcher->textures->getTemporaryTextureData(id);
		bool is64 = td && td->h >= 64;
		playerModel->setSkinIs64(is64);
	}
	super::render(e, x, y, z, rot, a);
}

void PlayerRenderer::setupPosition( Entity* mob, float x, float y, float z ) {
	Player* player = (Player*) mob;
	if(player->isAlive() && player->isSleeping()) {
		return super::setupPosition(mob, x + player->bedOffsetX, y + player->bedOffsetY, z + player->bedOffsetZ);
	}
	return super::setupPosition(mob, x, y, z);
}

void PlayerRenderer::setupRotations( Entity* mob, float bob, float bodyRot, float a ) {
	Player* player = (Player*) mob;
	if(player->isAlive() && player->isSleeping()) {
		glRotatef(player->getSleepRotation(), 0, 1, 0);
		glRotatef(getFlipDegrees(player), 0, 0, 1);
		glRotatef(270, 0, 1, 0);
		return;
	}
	super::setupRotations(mob, bob, bodyRot, a);
}

void PlayerRenderer::renderName( Mob* mob, float x, float y, float z ){
	//@todo: figure out how to handle HideGUI
	if (mob != entityRenderDispatcher->cameraEntity && mob->level->adventureSettings.showNameTags) {
		renderNameTag(mob, ((Player*)mob)->name, x, y, z, 32);
	}
}

int PlayerRenderer::prepareArmor(Mob* mob, int layer, float a) {
	Player* player = (Player*) mob;

	ItemInstance* itemInstance = player->getArmor(layer);
	if (!ItemInstance::isArmorItem(itemInstance))
		return -1;

	ArmorItem* armorItem = (ArmorItem*) itemInstance->getItem();
	int fnIndex = (armorItem->modelIndex + armorItem->modelIndex) + (layer == 2 ? 1 : 0);
	bindTexture(armorFilenames[fnIndex]);

	HumanoidModel* armor = layer == 2 ? armorParts2 : armorParts1;

	armor->head.visible = layer == 0;
	//armor.hair.visible = layer == 0;
	armor->body.visible = layer == 1 || layer == 2;
	armor->arm0.visible = layer == 1;
	armor->arm1.visible = layer == 1;
	armor->leg0.visible = layer == 2 || layer == 3;
	armor->leg1.visible = layer == 2 || layer == 3;

	setArmor(armor);

	/*if (itemInstance.isEnchanted())
		return 15; */

	return 1;
}

void PlayerRenderer::additionalRendering(Mob* mob, float a) {
	super::additionalRendering(mob, a);

	Player* player = (Player*) mob;
	const std::string& capeTexture = player->getCapeTexture();
	if (capeTexture.empty())
		return;

	bindTexture(capeTexture);
	glEnable2(GL_ALPHA_TEST);

	// ws is the same 0..1 walk-cycle speed MobRenderer uses to scale limb
	// swing, and walkAnimPos is the walk-cycle phase (advances only while
	// moving) — so this leans the cape back further the faster you're
	// walking/sprinting, plus a small per-step flutter, and settles back to
	// its resting angle as soon as you stop.
	float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a;
	if (ws > 1) ws = 1;
	if (ws < 0) ws = 0;
	float flutter = Mth::sin(mob->walkAnimPos * 0.6707f) * 4.0f * ws;
	float wind = (6.0f + ws * 26.0f + flutter) * Mth::DEGRAD;

	// Follow the body's current forward lean too (playerModel->body.xRot, set
	// by HumanoidModel::setupAnim() earlier this frame — 0.5 rad while
	// sneaking, 0 otherwise) so the cape bends together with the back instead
	// of staying at a fixed angle and clipping through a crouched player.
	capeModel->cape.xRot = playerModel->body.xRot + wind;
	capeModel->cape.render(1 / 16.0f);
}

void PlayerRenderer::onGraphicsReset() {
	super::onGraphicsReset();

	if (armorParts1) armorParts1->onGraphicsReset();
	if (armorParts2) armorParts2->onGraphicsReset();
	if (capeModel) capeModel->onGraphicsReset();
}
