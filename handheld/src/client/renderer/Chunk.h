#ifndef NET_MINECRAFT_CLIENT_RENDERER__Chunk_H__
#define NET_MINECRAFT_CLIENT_RENDERER__Chunk_H__

//package net.minecraft.client.renderer;

#include "RenderChunk.h"
#include "../../world/phys/AABB.h"

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
#include <vector>
#include <cstdint>
#include <atomic>
#endif

class Level;
class Entity;
class Culler;
class Tesselator;

// @note: TileEntity stuff is stripped away
class Chunk
{
    static const int NumLayers = 3;
public:
    Chunk(Level* level_, int x, int y, int z, int size, int lists_, GLuint* ptrBuf = NULL);

    void setPos(int x, int y, int z);

	void rebuild();
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	// Background-thread tessellation step: fills _cpuMesh[]/cpuVertexCount[]
	// using the provided Tesselator (not the global singleton).
	void rebuildCPU(Tesselator& t);
	// Main-thread GPU upload step: uploads _cpuMesh[] to OpenGL VBOs.
	void uploadGPU();
#endif
	void setDirty();
	void setClean();
	bool isDirty();
	void reset();

    float distanceToSqr(const Entity* player) const;
    float squishedDistanceToSqr(const Entity* player) const;

	//@todo @fix
    int getAllLists(int displayLists[], int p, int layer);
	int getList(int layer);

	RenderChunk& getRenderChunk(int layer);

	bool isEmpty();
    void cull(Culler* culler);

    void renderBB();
	static void resetUpdates();

private:
	void translateToPos();
	void rebuildImpl(Tesselator& t, bool cpuOnly);
public:
	Level* level;

	static int updates;// = 0;

	// Chunk mesh vertices are baked as (tileWorldCoord - originX/Z) rather
	// than raw absolute world coordinates, and the camera translate in
	// LevelRenderer subtracts the same origin before it ever reaches GL.
	// Near spawn this origin just sits at (0,0), a no-op. Far out (past
	// ~1M+ blocks) absolute float32 world coordinates and a per-frame
	// rotation-composed camera translate on them stop cancelling cleanly -
	// two independently-rounded huge values get subtracted, and since the
	// rounding pattern depends on the current view rotation, the result
	// visibly swims/flashes as the camera turns, even standing still.
	// Subtracting a shared origin keeps chunk-local and camera-relative
	// values small (bounded by the origin-shift threshold) so that never
	// happens. See
	// LevelRenderer::render()'s origin-shift check for where these move.
	static int originX, originZ;

	int x, y, z, xs, ys, zs;
	bool empty[NumLayers];
	int xm, ym, zm;
	float radius;
	AABB bb;

	int id;
	bool visible;
	bool occlusion_visible;
	bool occlusion_querying;
	int occlusion_id;
	bool queued; // true while this chunk is in LevelRenderer::dirtyChunks
	bool skyLit;

	RenderChunk renderChunk[NumLayers];

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	// CPU-side mesh buffers written by rebuildCPU(), consumed by uploadGPU().
	std::vector<uint8_t> _cpuMesh[NumLayers];
	int _cpuVertexCount[NumLayers];
	bool _cpuEmpty[NumLayers];
	bool _cpuAllEmpty;
	bool _cpuSkyLit;
	bool _cpuCompiled;
	// Set to true by rebuildCPU() when data is ready, cleared by uploadGPU().
	std::atomic<bool> _cpuMeshReady;
	// True while rebuildCPU() or uploadGPU() are executing (prevents double-queue).
	std::atomic<bool> _inFlight;
#endif

private:
	Tesselator& t;
	int lists;
	GLuint* vboBuffers;
	bool compiled;
	bool dirty;
    bool _empty;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__Chunk_H__*/
