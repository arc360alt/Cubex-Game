#ifndef NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__

#include "Model.h"
#include "../../util/Mth.h"

// A single flat flap hung behind a player's back, rendered as its own pass
// (with its own, separate-from-skin texture) after the body/armor layers —
// see PlayerRenderer::additionalRendering().
//
// Uses the standard vanilla (Java Edition) cape texture on a 64x32 image: the
// cape is the 22x17 region at the top left - a 1px top/bottom strip, then
// 1px-wide left/right edges flanking the 10x16 outside (at 1,1) and inside
// (at 12,1). That's exactly what a (w=10,h=16,d=1) box unfolds to under this
// engine's box-UV scheme starting at (0,0), so real cape files drop straight
// in (see backend/capes/README.md). The elytra on the right is unused.
//
// The box's "front" face (the cape's outside) points to -z, which is towards
// the player's back, so like vanilla the cape is turned around 180 degrees
// (yRot = PI). Because of that, xRot is applied in the turned-around frame:
// a *negative* xRot swings the cape out behind the player.
class CapeModel: public Model {
public:
	ModelPart cape;

	CapeModel() {
		texWidth = 64;
		texHeight = 32;
		cape.setModel(this);
		cape.texOffs(0, 0);
		// Hangs from the neck (y=0, same reference the body/head use) down to
		// about mid-thigh, sitting just behind the body's back face (z=2 once
		// turned around).
		cape.addBox(-5, 0, -3.0f, 10, 16, 1, 0.0f);
		cape.setPos(0, 0, 0);
		cape.yRot = Mth::PI;
		// Slight backward lean, hinged at the top (attachment point)
		cape.xRot = -6.0f * Mth::DEGRAD;
	}

	void onGraphicsReset() {
		cape.onGraphicsReset();
	}
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__*/
