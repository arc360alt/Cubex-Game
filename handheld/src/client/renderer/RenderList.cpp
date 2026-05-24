#include "RenderList.h"

#include "gles.h"
#include "RenderChunk.h"
#include "Tesselator.h"


RenderList::RenderList()
	:	inited(false),
	rendered(false)
{
	lists = new int[MAX_NUM_OBJECTS];
	rlists = new RenderChunk[MAX_NUM_OBJECTS];

	for (int i = 0; i < MAX_NUM_OBJECTS; ++i)
		rlists[i].vboId = -1;
}

RenderList::~RenderList() {
	delete[] lists;
	delete[] rlists;
}

void RenderList::init(float xOff, float yOff, float zOff) {
	inited = true;
	listIndex = 0;

	this->xOff = (float) xOff;
	this->yOff = (float) yOff;
	this->zOff = (float) zOff;
}

void RenderList::add(int list) {
	if (listIndex >= MAX_NUM_OBJECTS)
		return;

	lists[listIndex] = list;
}

void RenderList::addR(const RenderChunk& chunk) {
	if (listIndex >= MAX_NUM_OBJECTS)
		return;

	rlists[listIndex] = chunk;
}

void RenderList::render() {

	if (!inited) return;
	if (!rendered) {
		bufferLimit = listIndex;
		listIndex = 0;
		rendered = true;
	}
	if (listIndex < bufferLimit) {
		glPushMatrix2();
		glTranslatef2(-xOff, -yOff, -zOff);

		#ifndef USE_VBO
			glCallLists(bufferLimit, GL_UNSIGNED_INT, lists);
		#else
			renderChunks();
		#endif/*!USE_VBO*/

		glPopMatrix2();
	}
}

void RenderList::renderChunks() {
	const int Stride = VertexSizeBytes;

#if !defined(__APPLE__) || defined(MACOS)
	// Desktop: enable client-state arrays for VBO rendering (once per render pass)
	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState2(GL_COLOR_ARRAY);
#endif

	// Batch rendering: set up pointer state once, then draw multiple chunks
	GLuint lastVboId = (GLuint)-1;
	for (int i = 0; i < bufferLimit; ++i) {
		RenderChunk& rc = rlists[i];
		
		if (rc.vertexCount <= 0) continue;  // Skip empty chunks

		// Only bind if VBO changed (reduces state changes from 1 per chunk to ~1 per 10 chunks)
		if (rc.vboId != lastVboId) {
			glBindBuffer2(GL_ARRAY_BUFFER, rc.vboId);
			lastVboId = rc.vboId;
#if defined(__APPLE__) && !defined(MACOS)
			// iOS GLES2: set pointers only after bind
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, Stride, 0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, Stride, (GLvoid*)(3*4));
			glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, Stride, (GLvoid*)(5*4));
#else
			glVertexPointer2(3, GL_FLOAT, Stride, 0);
			glTexCoordPointer2(2, GL_FLOAT, Stride, (GLvoid*)(3*4));
			glColorPointer2(4, GL_UNSIGNED_BYTE, Stride, (GLvoid*)(5*4));
#endif
		}

#if defined(__APPLE__) && !defined(MACOS)
		shaderSyncUniforms();
#endif
		glDrawArrays2(GL_TRIANGLES, 0, rc.vertexCount);
	}

#if !defined(__APPLE__) || defined(MACOS)
	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState2(GL_COLOR_ARRAY);
#endif
}

void RenderList::clear() {
	inited = false;
	rendered = false;
}
