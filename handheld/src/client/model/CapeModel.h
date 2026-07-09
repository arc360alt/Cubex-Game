#ifndef NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__
#define NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__

#include "Model.h"
#include "../../util/Mth.h"

// A single flat flap hung behind a player's back, rendered as its own pass
// (with its own, separate-from-skin texture) after the body/armor layers —
// see PlayerRenderer::additionalRendering().
//
// Uses the standard vanilla cape UV layout on a 64x32 texture: a 1px top strip,
// a 1px bottom strip, then 1px-wide left/right edges flanking a 10x16 front
// and back. That's exactly what a (w=10,h=16,d=1) box unfolds to under this
// engine's box-UV scheme starting at (0,0), so any cape texture authored in
// that well-known format (see backend/capes/README.md) just drops in.
class CapeModel: public Model {
public:
	ModelPart cape;

	CapeModel() {
		texWidth = 64;
		texHeight = 32;
		cape.setModel(this);
		cape.texOffs(0, 0);
		// Hangs from the neck (y=0, same reference the body/head use) down to
		// about mid-thigh, sitting just behind the body's back face (z=2).
		cape.addBox(-5, 0, 2.0f, 10, 16, 1, 0.0f);
		cape.setPos(0, 0, 0);
		// Slight backward lean, hinged at the top (attachment point), purely
		// for looks — no walk-cycle swing animation in this first pass.
		cape.xRot = 6.0f * Mth::DEGRAD;
	}

	void onGraphicsReset() {
		cape.onGraphicsReset();
	}
};

#endif /*NET_MINECRAFT_CLIENT_MODEL__CapeModel_H__*/
