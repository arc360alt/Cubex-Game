#include "HumanoidModel.h"
#include "../Minecraft.h"
#include "../../util/Mth.h"
#include "../../world/entity/player/Player.h"
#include "../../world/entity/player/Inventory.h"
#include "../renderer/entity/EntityRenderDispatcher.h"
#include "../renderer/gles.h"
#include "../Options.h"

// Rebuild every model part for the given skin format and arm width.
// Separating this into a free function keeps the constructor and setSkinIs64()
// in sync without code duplication.
static void reinitParts(HumanoidModel* m, bool is64, bool isAlex) {
    m->texHeight = is64 ? 64 : 32;

    const float g   = m->g_val;
    const float yo  = m->yOffset_val;
    const float ov  = g + 0.25f; // outer-layer inflation (matches vanilla 1.8)

    // Helper: clear cubes, force recompile, re-register with model, set UV offset.
#define RESETPART(p, u, v) \
    m->p.clear(); m->p.onGraphicsReset(); m->p.setModel(m); m->p.texOffs(u, v)

    // ---- HEAD (UV identical in both skin formats) ----
    RESETPART(head, 0, 0);
    m->head.addBox(-4, -8, -4, 8, 8, 8, g);
    m->head.setPos(0, yo, 0);

    // ---- BODY (UV identical) ----
    RESETPART(body, 16, 16);
    m->body.addBox(-4, 0, -2, 8, 12, 4, g);
    m->body.setPos(0, yo, 0);

    // ---- RIGHT ARM ----
    RESETPART(arm0, 40, 16);
    if (isAlex) {
        m->arm0.addBox(-2, -2, -2, 3, 12, 4, g);
        m->arm0.setPos(-5, 2.5f + yo, 0);
    } else {
        m->arm0.addBox(-3, -2, -2, 4, 12, 4, g);
        m->arm0.setPos(-5, 2 + yo, 0);
    }

    // ---- LEFT ARM ----
    // 64x64: dedicated UV area at (32,48), no X-mirror needed.
    // 64x32: shares UV with right arm, mirrored.
    m->arm1.clear(); m->arm1.onGraphicsReset(); m->arm1.setModel(m);
    if (is64) {
        m->arm1.mirror = false;
        m->arm1.texOffs(32, 48);
    } else {
        m->arm1.mirror = true;
        m->arm1.texOffs(40, 16);
    }
    if (isAlex) {
        m->arm1.addBox(-1, -2, -2, 3, 12, 4, g);
        m->arm1.setPos(5, 2.5f + yo, 0);
    } else {
        m->arm1.addBox(-1, -2, -2, 4, 12, 4, g);
        m->arm1.setPos(5, 2 + yo, 0);
    }

    // ---- RIGHT LEG (UV identical) ----
    RESETPART(leg0, 0, 16);
    m->leg0.addBox(-2, 0, -2, 4, 12, 4, g);
    m->leg0.setPos(-2, 12 + yo, 0);

    // ---- LEFT LEG ----
    // 64x64: dedicated UV area at (16,48), no mirror.
    // 64x32: shares UV with right leg, mirrored.
    m->leg1.clear(); m->leg1.onGraphicsReset(); m->leg1.setModel(m);
    if (is64) {
        m->leg1.mirror = false;
        m->leg1.texOffs(16, 48);
    } else {
        m->leg1.mirror = true;
        m->leg1.texOffs(0, 16);
    }
    m->leg1.addBox(-2, 0, -2, 4, 12, 4, g);
    m->leg1.setPos(2, 12 + yo, 0);

    // ---- HAT / head overlay (UV 32,0 — same location in both skin formats) ----
    RESETPART(hat, 32, 0);
    m->hat.addBox(-4, -8, -4, 8, 8, 8, g + 0.5f);
    m->hat.setPos(0, yo, 0);
    m->hat.visible = true;

    // ---- OUTER LAYER (64x64 only) ----
    // Body overlay: UV (16,32)
    RESETPART(bodyOverlay, 16, 32);
    m->bodyOverlay.addBox(-4, 0, -2, 8, 12, 4, ov);
    m->bodyOverlay.setPos(0, yo, 0);
    m->bodyOverlay.visible = is64;

    // Right arm overlay: UV (40,32)
    RESETPART(arm0Overlay, 40, 32);
    if (isAlex) {
        m->arm0Overlay.addBox(-2, -2, -2, 3, 12, 4, ov);
        m->arm0Overlay.setPos(-5, 2.5f + yo, 0);
    } else {
        m->arm0Overlay.addBox(-3, -2, -2, 4, 12, 4, ov);
        m->arm0Overlay.setPos(-5, 2 + yo, 0);
    }
    m->arm0Overlay.visible = is64;

    // Left arm overlay: UV (48,48) — dedicated, no mirror
    m->arm1Overlay.clear(); m->arm1Overlay.onGraphicsReset(); m->arm1Overlay.setModel(m);
    m->arm1Overlay.mirror = false;
    m->arm1Overlay.texOffs(48, 48);
    if (isAlex) {
        m->arm1Overlay.addBox(-1, -2, -2, 3, 12, 4, ov);
        m->arm1Overlay.setPos(5, 2.5f + yo, 0);
    } else {
        m->arm1Overlay.addBox(-1, -2, -2, 4, 12, 4, ov);
        m->arm1Overlay.setPos(5, 2 + yo, 0);
    }
    m->arm1Overlay.visible = is64;

    // Right leg overlay: UV (0,32)
    RESETPART(leg0Overlay, 0, 32);
    m->leg0Overlay.addBox(-2, 0, -2, 4, 12, 4, ov);
    m->leg0Overlay.setPos(-2, 12 + yo, 0);
    m->leg0Overlay.visible = is64;

    // Left leg overlay: UV (0,48) — no mirror
    m->leg1Overlay.clear(); m->leg1Overlay.onGraphicsReset(); m->leg1Overlay.setModel(m);
    m->leg1Overlay.mirror = false;
    m->leg1Overlay.texOffs(0, 48);
    m->leg1Overlay.addBox(-2, 0, -2, 4, 12, 4, ov);
    m->leg1Overlay.setPos(2, 12 + yo, 0);
    m->leg1Overlay.visible = is64;

#undef RESETPART

    m->skinIs64 = is64;
}

HumanoidModel::HumanoidModel( float g /*= 0*/, float yOffset /*= 0*/ )
:	g_val(g),
	yOffset_val(yOffset),
	lastSkinType(0),
	skinIs64(false),
	holdingLeftHand(false),
	holdingRightHand(false),
	sneaking(false),
	bowAndArrow(false)
{
	::reinitParts(this, false, false); // default: 64x32 Steve
}

void HumanoidModel::setSkinIs64(bool is64) {
	if (is64 == skinIs64) return;
	::reinitParts(this, is64, lastSkinType == 1);
}

void HumanoidModel::reinitArms(bool isAlex) {
	// Rebuild all parts so arm geometry (Steve/Alex) and overlay parts stay consistent.
	::reinitParts(this, skinIs64, isAlex);
}

void HumanoidModel::checkSkinType() {
	EntityRenderDispatcher* erd = EntityRenderDispatcher::getInstance();
	if (erd && erd->options) {
		int currentSkinType = erd->options->alexSkin ? 1 : 0;
		if (currentSkinType != lastSkinType) {
			lastSkinType = currentSkinType;
			reinitArms(currentSkinType == 1);
		}
	}
}

// Sync an overlay part's position/rotation from its corresponding base part,
// then render it.  The overlay uses blending already set by the caller.
static void renderOverlay(ModelPart& overlay, const ModelPart& base, float scale) {
	if (!overlay.visible) return;
	overlay.x    = base.x;    overlay.y    = base.y;    overlay.z    = base.z;
	overlay.xRot = base.xRot; overlay.yRot = base.yRot; overlay.zRot = base.zRot;
	overlay.render(scale);
}

void HumanoidModel::render(Entity* e, float time, float r, float bob, float yRot, float xRot, float scale )
{
	checkSkinType();
	if(e != NULL && e->isMob()) {
		Mob* mob = (Mob*)(e);
		ItemInstance* item = mob->getCarriedItem();
		if(item != NULL) {
			if(mob->getUseItemDuration() >  0) {
				UseAnim::UseAnimation anim = item->getUseAnimation();
				if(anim == UseAnim::bow) {
					bowAndArrow = true;
				}
			}
		}
	}

	setupAnim(time, r, bob, yRot, xRot, scale);

	// --- Base skin layer ---
	head.render(scale);
	body.render(scale);
	arm0.render(scale);
	arm1.render(scale);
	leg0.render(scale);
	leg1.render(scale);

	// --- Outer skin layer (hat + overlays) with alpha blending ---
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderOverlay(hat,         head, scale);
	renderOverlay(bodyOverlay, body, scale);
	renderOverlay(arm0Overlay, arm0, scale);
	renderOverlay(arm1Overlay, arm1, scale);
	renderOverlay(leg0Overlay, leg0, scale);
	renderOverlay(leg1Overlay, leg1, scale);

	glDisable2(GL_BLEND);

	bowAndArrow = false;
}

void HumanoidModel::render( HumanoidModel* model, float scale )
{
	checkSkinType();
	head.yRot = model->head.yRot;
	head.y = model->head.y;
	head.xRot = model->head.xRot;

	arm0.xRot = model->arm0.xRot;
	arm0.zRot = model->arm0.zRot;

	arm1.xRot = model->arm1.xRot;
	arm1.zRot = model->arm1.zRot;

	leg0.xRot = model->leg0.xRot;
	leg1.xRot = model->leg1.xRot;

	head.render(scale);
	body.render(scale);
	arm0.render(scale);
	arm1.render(scale);
	leg0.render(scale);
	leg1.render(scale);

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	renderOverlay(hat,         head, scale);
	renderOverlay(bodyOverlay, body, scale);
	renderOverlay(arm0Overlay, arm0, scale);
	renderOverlay(arm1Overlay, arm1, scale);
	renderOverlay(leg0Overlay, leg0, scale);
	renderOverlay(leg1Overlay, leg1, scale);

	glDisable2(GL_BLEND);
}

void HumanoidModel::renderHorrible( float time, float r, float bob, float yRot, float xRot, float scale )
{
	checkSkinType();
	setupAnim(time, r, bob, yRot, xRot, scale);
	head.renderHorrible(scale);
	body.renderHorrible(scale);
	arm0.renderHorrible(scale);
	arm1.renderHorrible(scale);
	leg0.renderHorrible(scale);
	leg1.renderHorrible(scale);
}

// Updated to match Minecraft Java, all except hair.
void HumanoidModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	head.yRot = yRot / (180 / Mth::PI);
	head.xRot = xRot / (180 / Mth::PI);
	const float tcos0 = Mth::cos(time * 0.6662f) * r;
	const float tcos1 = Mth::cos(time * 0.6662f + Mth::PI) * r;

	arm0.xRot = tcos1;
	arm1.xRot = tcos0;
	arm0.zRot = 0;
	arm1.zRot = 0;

	leg0.xRot = tcos0 * 1.4f;
	leg1.xRot = tcos1 * 1.4f;
	leg0.yRot = 0;
	leg1.yRot = 0;

	if (riding) {
		arm0.xRot += -Mth::PI / 2 * 0.4f;
		arm1.xRot += -Mth::PI / 2 * 0.4f;
		leg0.xRot = -Mth::PI / 2 * 0.8f;
		leg1.xRot = -Mth::PI / 2 * 0.8f;
		leg0.yRot = Mth::PI / 2 * 0.2f;
		leg1.yRot = -Mth::PI / 2 * 0.2f;
	}

	if (holdingLeftHand != 0) {
		arm1.xRot = arm1.xRot * 0.5f - Mth::PI / 2.0f * 0.2f * holdingLeftHand;
	}
	if (holdingRightHand != 0) {
		arm0.xRot = arm0.xRot * 0.5f - Mth::PI / 2.0f * 0.2f * holdingRightHand;
	}
	arm0.yRot = 0;
	arm1.yRot = 0;

	if (attackTime > -9990) {
		float swing = attackTime;
		body.yRot = Mth::sin(Mth::sqrt(swing) * Mth::PI * 2) * 0.2f;
		arm0.z = Mth::sin(body.yRot) * 5;
		arm0.x = -Mth::cos(body.yRot) * 5;
		arm1.z = -Mth::sin(body.yRot) * 5;
		arm1.x = Mth::cos(body.yRot) * 5;
		arm0.yRot += body.yRot;
		arm1.yRot += body.yRot;
		arm1.xRot += body.yRot;

		swing = 1 - attackTime;
		swing *= swing;
		swing *= swing;
		swing = 1 - swing;
		float aa = Mth::sin(swing * Mth::PI);
		float bb = Mth::sin(attackTime * Mth::PI) * -(head.xRot - 0.7f) * 0.75f;
		arm0.xRot -= aa * 1.2f + bb;
		arm0.yRot += body.yRot * 2;
		arm0.zRot = Mth::sin(attackTime * Mth::PI) * -0.4f;
	}

	if (sneaking) {
		body.xRot = 0.5f;
		arm0.xRot += 0.4f;
		arm1.xRot += 0.4f;
		leg0.z = +4.0f;
		leg1.z = +4.0f;
		leg0.y = +9.0f;
		leg1.y = +9.0f;
		head.y = +1;
	} else {
		body.xRot = 0.0f;
		leg0.z = +0.0f;
		leg1.z = +0.0f;
		leg0.y = +12.0f;
		leg1.y = +12.0f;
		head.y = 0;
	}

	const float bcos = Mth::cos(bob * 0.09f) * 0.05f + 0.05f;
	const float bsin = Mth::sin(bob * 0.067f) * 0.05f;

	arm0.zRot += bcos;
	arm1.zRot -= bcos;
	arm0.xRot += bsin;
	arm1.xRot -= bsin;

	if (bowAndArrow) {
		float attack2 = 0;
		float attack = 0;

		arm0.zRot = 0;
		arm1.zRot = 0;
		arm0.yRot = -(0.1f - attack2 * 0.6f) + head.yRot;
		arm1.yRot = +(0.1f - attack2 * 0.6f) + head.yRot + 0.4f;
		arm0.xRot = -Mth::PI / 2.0f + head.xRot;
		arm1.xRot = -Mth::PI / 2.0f + head.xRot;
		arm0.xRot -= attack2 * 1.2f - attack * 0.4f;
		arm1.xRot -= attack2 * 1.2f - attack * 0.4f;

		arm0.zRot += bcos;
		arm1.zRot -= bcos;
		arm0.xRot += bsin;
		arm1.xRot -= bsin;
	}
}

void HumanoidModel::onGraphicsReset()
{
	head.onGraphicsReset();
	body.onGraphicsReset();
	arm0.onGraphicsReset();
	arm1.onGraphicsReset();
	leg0.onGraphicsReset();
	leg1.onGraphicsReset();
	hat.onGraphicsReset();
	bodyOverlay.onGraphicsReset();
	arm0Overlay.onGraphicsReset();
	arm1Overlay.onGraphicsReset();
	leg0Overlay.onGraphicsReset();
	leg1Overlay.onGraphicsReset();
}
