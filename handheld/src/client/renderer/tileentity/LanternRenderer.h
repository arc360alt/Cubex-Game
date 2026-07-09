#ifndef NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__LanternRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__LanternRenderer_H__

//package net.minecraft.client.renderer.tileentity;

#include "TileEntityRenderer.h"

// Draws the lantern as a simple 4-sided box textured from lantern.png -
// entirely separate from the chunk mesh/terrain.png (see LanternTile).
class LanternRenderer: public TileEntityRenderer
{
public:
	/*@Override*/
	void render(TileEntity* te, float x, float y, float z, float a);
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER_TILEENTITY__LanternRenderer_H__*/
