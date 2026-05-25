#include "ChestRenderer.h"
#include "TileEntityRenderer.h"
#include "../gles.h"
#include "../../model/ChestModel.h"
#include "../../../world/level/tile/entity/ChestTileEntity.h"
#include "../../../world/level/tile/ChestTile.h"
#include "../../../util/Mth.h"


void ChestRenderer::render( TileEntity* entity, float x, float y, float z, float a )
{
	ChestTileEntity* chest = (ChestTileEntity*) entity;
	int data = 0;

	if (chest->level) {
		Tile* tile = chest->getTile();
		data = chest->getData();

		if (tile != NULL && data == 0) {
			((ChestTile*)tile)->recalcLockDir(chest->level, chest->x, chest->y, chest->z);
			data = chest->getData();
		}

		chest->checkNeighbors();
	}

	// Secondary half of a double chest — the primary half renders for both.
	if (chest->n != NULL || chest->w != NULL) return;

	const bool isDouble = (chest->e != NULL || chest->s != NULL);
	bindTexture("item/chest.png");

	glPushMatrix2();
	glColor4f2(1, 1, 1, 1);
	glTranslatef2(x, y + 1, z + 1);
	glScalef2(1, -1, -1);

	glTranslatef2(0.5f, 0.5f, 0.5f);

	GLfloat rot = 0;
	if (data == 2) rot = 180;
	if (data == 3) rot = 0;
	if (data == 4) rot = 90;
	if (data == 5) rot = -90;

	// For double chests the large model always extends in model +X.
	// Two cases need a pre-rotation shift to align it with the correct blocks:
	//   facing NORTH (rot=180, E-W pair): model +X maps world -X, shift +1 in post-rot X
	//   facing EAST  (rot=-90, N-S pair): model +X maps world -Z, shift -1 in post-rot Z
	if (isDouble) {
		if (data == 2)      glTranslatef2(1.0f, 0.0f,  0.0f);
		else if (data == 5) glTranslatef2(0.0f, 0.0f, -1.0f);
	}

	glRotatef2(rot, 0, 1, 0);
	glTranslatef2(-0.5f, -0.5f, -0.5f);

	float open = chest->oOpenness + (chest->openness - chest->oOpenness) * a;
	if (chest->e != NULL) {
		float open2 = chest->e->oOpenness + (chest->e->openness - chest->e->oOpenness) * a;
		if (open2 > open) open = open2;
	}
	if (chest->s != NULL) {
		float open2 = chest->s->oOpenness + (chest->s->openness - chest->s->oOpenness) * a;
		if (open2 > open) open = open2;
	}

	open = 1 - open;
	open = 1 - open * open * open;

	if (isDouble) {
		largeChestModel.lid.xRot = -(open * Mth::PI / 2);
		largeChestModel.render();
	} else {
		chestModel.lid.xRot = -(open * Mth::PI / 2);
		chestModel.render();
	}
	glPopMatrix2();
	glColor4f2(1, 1, 1, 1);
}
