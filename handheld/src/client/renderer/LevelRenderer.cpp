#include "LevelRenderer.h"

#include "DirtyChunkSorter.h"
#include "DistanceChunkSorter.h"
#include "Chunk.h"
#include "Tesselator.h"
#include "TileRenderer.h"
#include "../Minecraft.h"
#include "../../util/Mth.h"
#include "../../world/entity/player/Player.h"
#include "../../world/level/tile/LevelEvent.h"
#include "../../world/level/tile/LeafTile.h"
#include "../../client/particle/ParticleEngine.h"
#include "../../client/particle/ParticleInclude.h"
#include "../sound/SoundEngine.h"
#include "culling/Culler.h"
#include "entity/EntityRenderDispatcher.h"
#include "../model/HumanoidModel.h"
#include <chrono>

#include "GameRenderer.h"
#include "../../AppPlatform.h"
#include "../../util/PerfTimer.h"
#include "Textures.h"
#include "tileentity/TileEntityRenderDispatcher.h"
#include "../particle/BreakingItemParticle.h"

#include "../../client/player/LocalPlayer.h"

#ifdef GFX_SMALLER_CHUNKS
/* static */ const int LevelRenderer::CHUNK_SIZE = 8;
#else
/* static */ const int LevelRenderer::CHUNK_SIZE = 16;
#endif


LevelRenderer::LevelRenderer( Minecraft* mc)
:	mc(mc),
	textures(mc->textures),
	level(NULL),
	cullStep(0),

	chunkLists(0),
	xChunks(0), yChunks(0), zChunks(0),

	chunks(NULL),
	sortedChunks(NULL),

	xMinChunk(0), yMinChunk(0), zMinChunk(0),
	xMaxChunk(0), yMaxChunk(0), zMaxChunk(0),

	lastViewDistance(-1),

	noEntityRenderFrames(2),
	totalEntities(0),
	renderedEntities(0),
	culledEntities(0),

	occlusionCheck(false),
	totalChunks(0), offscreenChunks(0), renderedChunks(0), occludedChunks(0), emptyChunks(0),

	chunkFixOffs(0),
	xOld(-9999), yOld(-9999), zOld(-9999),

	ticks(0),
	skyList(0), starList(0), darkList(0),
	tileRenderer(NULL),
	destroyProgress(0)
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	, buildThreadsActive(false)
#endif
{
	_skyChangedPending = false;
#ifdef USE_VBO
	// Pool must cover the largest possible dist. allChanged() caps dist at 1024 for
	// macOS/Linux and 400 for other platforms. Use the per-platform cap to size exactly.
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	static const int MAX_RENDER_DIST = 1024;
#else
	static const int MAX_RENDER_DIST = 256;
#endif
	{
		int maxW = MAX_RENDER_DIST / CHUNK_SIZE + 1;
		numListsOrBuffers = maxW * maxW * (128/CHUNK_SIZE) * 3;
	}
	chunkBuffers = new GLuint[numListsOrBuffers];
	glGenBuffers2(numListsOrBuffers, chunkBuffers);
	LOGI("numBuffers: %d\n", numListsOrBuffers);
#   if defined(OPENGL_ES) || defined(MACOS) || defined(LINUX)
	glGenBuffers2(1, &skyBuffer);
	generateSky();
#   endif
#else
	int maxChunksWidth = 1024 / CHUNK_SIZE;
	numListsOrBuffers = maxChunksWidth * maxChunksWidth * maxChunksWidth * 3;
	chunkLists = glGenLists(numListsOrBuffers);
#endif

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	startBuildThreads();
#endif
}

LevelRenderer::~LevelRenderer()
{
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	stopBuildThreadsAndDrainQueues();
#endif

	delete tileRenderer;
	tileRenderer = NULL;

	deleteChunks();

#ifdef USE_VBO
	glDeleteBuffers(numListsOrBuffers, chunkBuffers);
#   if defined(OPENGL_ES) || defined(MACOS) || defined(LINUX)
	glDeleteBuffers(1, &skyBuffer);
#   endif
	delete[] chunkBuffers;
#else
	glDeleteLists(numListsOrBuffers, chunkLists);
#endif

}

void LevelRenderer::generateSky() {
	Tesselator& t = Tesselator::instance;
	float yy;
	int s = 128;
	int d = (256 / s) + 2;
	yy = (float) (16);
	t.begin();

	skyVertexCount = 0;
	for (int xx = -s * d; xx <= s * d; xx += s) {
		for (int zz = -s * d; zz <= s * d; zz += s) {
			t.vertex((float) xx + 0,  yy, (float) zz + 0);
			t.vertex((float)(xx + s), yy, (float) zz + 0);
			t.vertex((float)(xx + s), yy, (float)(zz + s));
			t.vertex((float) xx + 0,  yy, (float)(zz + s));
			//LOGI("x, z: %d, %d\n", xx, zz);
			skyVertexCount += 4;
		}
	}

	t.end(true, skyBuffer);
	//LOGI("skyvertexcount: %d\n", skyVertexCount);
	//glEndList();
}

void LevelRenderer::setLevel( Level* level )
{
	if (this->level != NULL) {
		this->level->removeListener(this);
	}

	xOld = -9999;
	yOld = -9999;
	zOld = -9999;

	EntityRenderDispatcher::getInstance()->setLevel(level);
	EntityRenderDispatcher::getInstance()->setMinecraft(mc);
	this->level = level;

	delete tileRenderer;
	tileRenderer = new TileRenderer(level);

	if (level != NULL) {
		level->addListener(this);
		allChanged();
	} else {
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
		stopBuildThreadsAndDrainQueues();
#endif
		dirtyChunks.clear();
		deleteChunks();
	}
}

void LevelRenderer::allChanged()
{
#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	stopBuildThreadsAndDrainQueues();
#endif

	deleteChunks();

#ifdef USE_VBO
	// Flush GPU pipeline so no in-flight draws reference old VBO data,
	// then orphan every buffer so MetalANGLE releases stale Metal backing.
	// This prevents GPU page faults when render distance changes cause a
	// different number of chunks to reuse the same VBO pool.
	glFinish();
	for (int i = 0; i < numListsOrBuffers; ++i) {
		glBindBuffer2(GL_ARRAY_BUFFER, chunkBuffers[i]);
		glBufferData2(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
	}
	glBindBuffer2(GL_ARRAY_BUFFER, 0);
#endif

	Tile::leaves->setFancy(mc->options.fancyGraphics);
	Tile::leaves_carried->setFancy(mc->options.fancyGraphics);
	lastViewDistance = mc->options.viewDistance & 7; // clamp to [0,7] — 0=far, 7=tiny

	// dist table: vd 0..7 → block distances (capped by maxChunksWidth in practice)
	static const int DIST_TABLE[8] = {512, 256, 128, 64, 768, 1024, 1536, 2048};
	int dist = DIST_TABLE[lastViewDistance];
	if (lastViewDistance <= 2 && mc->isPowerVR())
		dist = (int)((float)dist * 0.8f);
	LOGI("last: %d, power: %d\n", lastViewDistance, mc->isPowerVR());

	#if defined(RPI)
		dist *= 0.6f;
	#endif

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	if (dist > 1024) dist = 1024;
#else
	if (dist > 256) dist = 256;
#endif
	/*
	* if (Minecraft.FLYBY_MODE) { dist = 512 - CHUNK_SIZE * 2; }
	*/
	xChunks = (dist / LevelRenderer::CHUNK_SIZE) + 1;
	yChunks = (128 /  LevelRenderer::CHUNK_SIZE);
	zChunks = (dist / LevelRenderer::CHUNK_SIZE) + 1;
	chunksLength = xChunks * yChunks * zChunks;
	LOGI("chunksLength: %d. Distance: %d\n", chunksLength, dist);

	chunks = new Chunk*[chunksLength];
	sortedChunks = new Chunk*[chunksLength];

	int id = 0;
	int count = 0;

	xMinChunk = 0;
	yMinChunk = 0;
	zMinChunk = 0;
	xMaxChunk = xChunks;
	yMaxChunk = yChunks;
	zMaxChunk = zChunks;
	dirtyChunks.clear();
	//renderableTileEntities.clear();

	for (int x = 0; x < xChunks; x++) {
		for (int y = 0; y < yChunks; y++) {
			for (int z = 0; z < zChunks; z++) {
				const int c = getLinearCoord(x, y, z);
				Chunk* chunk = new Chunk(level, x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE, CHUNK_SIZE, chunkLists + id, &chunkBuffers[id]);

				if (occlusionCheck) {
					chunk->occlusion_id = 0;//occlusionCheckIds.get(count);
				}
				chunk->occlusion_querying = false;
				chunk->occlusion_visible = true;
				chunk->visible = true;
				chunk->id = count++;
				chunk->setDirty();

				chunks[c] = chunk;
				sortedChunks[c] = chunk;
				chunk->queued = true;
				dirtyChunks.push_back(chunk);

				id += 3;
			}
		}
	}

	if (level != NULL) {
		Entity* player = mc->cameraTargetPlayer;
		if (player != NULL) {
			this->resortChunks(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
			DistanceChunkSorter distanceSorter(player);
			std::sort(sortedChunks, sortedChunks + chunksLength, distanceSorter);
		}
	}
	noEntityRenderFrames = 2;

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	if (level != NULL) {
		startBuildThreads();
	}
#endif
}

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
void LevelRenderer::startBuildThreads()
{
	if (buildThreadsActive.load(std::memory_order_acquire)) {
		return;
	}

	buildThreadsActive.store(true, std::memory_order_release);

	// Scale to hardware: leave 1 core for main/render thread, cap at 8
	unsigned int hw = std::thread::hardware_concurrency();
	int numThreads = (hw > 1) ? (int)std::min(hw - 1u, 8u) : 1;

	const int THREAD_TESS_SIZE = 16 * 1024 * 1024;
	threadTesselators.reserve(numThreads);
	for (int i = 0; i < numThreads; ++i) {
		threadTesselators.push_back(std::unique_ptr<Tesselator>(new Tesselator(THREAD_TESS_SIZE)));
		buildThreads.emplace_back(&LevelRenderer::buildThreadWorker, this, i);
	}
	LOGI("Started %d chunk build threads\n", numThreads);
}

void LevelRenderer::stopBuildThreadsAndDrainQueues()
{
	if (buildThreadsActive.load(std::memory_order_acquire)) {
		buildThreadsActive.store(false, std::memory_order_release);
	}

	// Wake all sleeping workers so they see buildThreadsActive=false and exit
	_buildWorkCV.notify_all();

	for (auto& thread : buildThreads) {
		if (thread.joinable()) {
			thread.join();
		}
	}
	buildThreads.clear();

	{
		std::lock_guard<std::mutex> lock(buildQueueMutex);
		while (!buildQueue.empty()) {
			Chunk* chunk = buildQueue.front();
			buildQueue.pop();
			if (chunk) {
				chunk->_inFlight.store(false, std::memory_order_release);
				chunk->_cpuMeshReady.store(false, std::memory_order_release);
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(uploadQueueMutex);
		while (!uploadQueue.empty()) {
			Chunk* chunk = uploadQueue.front();
			uploadQueue.pop();
			if (chunk) {
				chunk->_inFlight.store(false, std::memory_order_release);
				chunk->_cpuMeshReady.store(false, std::memory_order_release);
			}
		}
	}

	threadTesselators.clear();
	LOGI("Stopped chunk build threads\n");
}

void LevelRenderer::buildThreadWorker(int threadIndex)
{
	// Get the pre-allocated Tesselator for this thread
	Tesselator* threadTess = threadTesselators[threadIndex].get();
	if (!threadTess) {
		LOGI("ERROR: Worker thread %d has no tesselator!\n", threadIndex);
		return;
	}
	
	while (true) {
		Chunk* chunk = nullptr;
		{
			std::unique_lock<std::mutex> lk(buildQueueMutex);
			_buildWorkCV.wait(lk, [this]{
				return !buildQueue.empty() || !buildThreadsActive.load(std::memory_order_relaxed);
			});
			if (!buildThreadsActive.load(std::memory_order_relaxed) && buildQueue.empty())
				break;
			chunk = buildQueue.front();
			buildQueue.pop();
		}
		chunk->rebuildCPU(*threadTess);
		{
			std::lock_guard<std::mutex> lk(uploadQueueMutex);
			uploadQueue.push(chunk);
		}
	}
}

void LevelRenderer::processUploadQueue()
{
	int pendingUploads = 0;
	{
		std::lock_guard<std::mutex> lock(uploadQueueMutex);
		pendingUploads = (int)uploadQueue.size();
	}

	if (pendingUploads <= 0)
		return;

	// Time-sliced uploads prevent frame spikes while still draining backlog quickly.
	const int maxUploads = (pendingUploads > 512) ? 64 : ((pendingUploads > 128) ? 40 : 24);
	const int minUploads = 2;
	const double budgetMs = (pendingUploads > 512) ? 5.0 : ((pendingUploads > 128) ? 3.5 : 2.0);
	const auto start = std::chrono::steady_clock::now();

	int uploadsThisFrame = 0;
	while (uploadsThisFrame < maxUploads) {
		Chunk* chunk = nullptr;
		{
			std::lock_guard<std::mutex> lock(uploadQueueMutex);
			if (!uploadQueue.empty()) {
				chunk = uploadQueue.front();
				uploadQueue.pop();
			}
		}
		if (!chunk) break;

		if (chunk->_cpuMeshReady.load(std::memory_order_acquire)) {
			chunk->uploadGPU();
			chunk->setClean();
			++uploadsThisFrame;
		} else {
			// Safety: unexpected state — release inFlight so chunk can be rebuilt
			chunk->_inFlight.store(false, std::memory_order_release);
			if (!chunk->isDirty()) chunk->setDirty();
			if (!chunk->queued) {
				chunk->queued = true;
				dirtyChunks.push_back(chunk);
			}
		}

		if (uploadsThisFrame >= minUploads) {
			auto now = std::chrono::steady_clock::now();
			double elapsedMs = std::chrono::duration<double, std::milli>(now - start).count();
			if (elapsedMs >= budgetMs)
				break;
		}
	}
}
#endif

void LevelRenderer::deleteChunks()
{
	if (!chunks) return;

	for (int z = 0; z < zChunks; ++z)
	for (int y = 0; y < yChunks; ++y)
	for (int x = 0; x < xChunks; ++x) {
		int c = getLinearCoord(x, y, z);
		delete chunks[c];
	}

	delete[] chunks;
	chunks = NULL;

	delete[] sortedChunks;
	sortedChunks = NULL;
}

void LevelRenderer::resortChunks( int xc, int yc, int zc )
{
	xc -= CHUNK_SIZE / 2;
	//yc -= CHUNK_SIZE / 2;
	zc -= CHUNK_SIZE / 2;
	xMinChunk = INT_MAX;
	yMinChunk = INT_MAX;
	zMinChunk = INT_MAX;
	xMaxChunk = INT_MIN;
	yMaxChunk = INT_MIN;
	zMaxChunk = INT_MIN;

	int dirty = 0;

	int s2 = xChunks * CHUNK_SIZE;
	int s1 = s2 / 2;

	for (int x = 0; x < xChunks; x++) {
		int xx = x * CHUNK_SIZE;

		int xOff = (xx + s1 - xc);
		if (xOff < 0) xOff -= (s2 - 1);
		xOff /= s2;
		xx -= xOff * s2;

		if (xx < xMinChunk) xMinChunk = xx;
		if (xx > xMaxChunk) xMaxChunk = xx;

		for (int z = 0; z < zChunks; z++) {
			int zz = z * CHUNK_SIZE;
			int zOff = (zz + s1 - zc);
			if (zOff < 0) zOff -= (s2 - 1);
			zOff /= s2;
			zz -= zOff * s2;

			if (zz < zMinChunk) zMinChunk = zz;
			if (zz > zMaxChunk) zMaxChunk = zz;

			for (int y = 0; y < yChunks; y++) {
				int yy = y * CHUNK_SIZE;
				if (yy < yMinChunk) yMinChunk = yy;
				if (yy > yMaxChunk) yMaxChunk = yy;

				Chunk* chunk = chunks[(z * yChunks + y) * xChunks + x];
				bool wasDirty = chunk->isDirty();
				chunk->setPos(xx, yy, zz);
				if (!wasDirty && chunk->isDirty()) {
					chunk->queued = true;
					dirtyChunks.push_back(chunk);
					++dirty;
				}
			}
		}
	}
}

int LevelRenderer::render( Mob* player, int layer, float alpha )
{
	if (mc->options.viewDistance != lastViewDistance) {
		allChanged();
	}

	TIMER_PUSH("sortchunks");
	if (layer == 0) {
		for (int i = 0; i < 10; i++) {
			chunkFixOffs = (chunkFixOffs + 1) % chunksLength;
			Chunk* c = chunks[chunkFixOffs];
			if (c->isDirty() && !c->queued) {
				c->queued = true;
				dirtyChunks.push_back(c);
			}
		}
	}

	if (layer == 0) {
		totalChunks = 0;
		offscreenChunks = 0;
		occludedChunks = 0;
		renderedChunks = 0;
		emptyChunks = 0;
	}

	if (layer == 0) {
		// Chunk mesh data and the camera translate below are both baked
		// relative to Chunk::originX/Z rather than absolute world
		// coordinates (see the comment on Chunk::originX) - keep that
		// origin tracking the player so those values stay small. This is a
		// no-op near spawn (origin just sits at 0,0); it only starts doing
		// anything once the player wanders far enough that absolute float32
		// world coordinates would otherwise lose the precision needed for
		// stable rendering. Done before xOff/yOff/zOff below so this
		// frame's render already uses the up-to-date origin.
		const int ORIGIN_SHIFT_THRESHOLD = 512;
		int px = Mth::floor(player->x);
		int pz = Mth::floor(player->z);
		if (Mth::abs(px - Chunk::originX) > ORIGIN_SHIFT_THRESHOLD ||
			Mth::abs(pz - Chunk::originZ) > ORIGIN_SHIFT_THRESHOLD) {
			Chunk::originX = (px / ORIGIN_SHIFT_THRESHOLD) * ORIGIN_SHIFT_THRESHOLD;
			Chunk::originZ = (pz / ORIGIN_SHIFT_THRESHOLD) * ORIGIN_SHIFT_THRESHOLD;
			for (int i = 0; i < chunksLength; i++) chunks[i]->setDirty();
		}
	}

	float xOff = player->xOld + (player->x - player->xOld) * alpha - Chunk::originX;
	float yOff = player->yOld + (player->y - player->yOld) * alpha;
	float zOff = player->zOld + (player->z - player->zOld) * alpha - Chunk::originZ;

	if (layer == 0) {
		static int lastResortTick = -999999;
		const float resortDist = float(CHUNK_SIZE) * 0.75f;
		float xd = player->x - xOld;
		float yd = player->y - yOld;
		float zd = player->z - zOld;
		bool movedEnough = (xd * xd + yd * yd + zd * zd) > (resortDist * resortDist);
		bool timedRefresh = (ticks - lastResortTick) >= 10;
		if (movedEnough || timedRefresh) {
			xOld = player->x;
			yOld = player->y;
			zOld = player->z;
			lastResortTick = ticks;

			resortChunks(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));
			DistanceChunkSorter distanceSorter(player);
			std::sort(sortedChunks, sortedChunks + chunksLength, distanceSorter);
		}
	}

	int count = 0;
	if (occlusionCheck && !mc->options.anaglyph3d && layer == 0) {
		int from = 0;
		int to = 16;
		//checkQueryResults(from, to);
		for (int i = from; i < to; i++) {
			sortedChunks[i]->occlusion_visible = true;
		}

		count += renderChunks(from, to, layer, alpha);

		do {
			from = to;
			to = to * 2;
			if (to > chunksLength) to = chunksLength;

			glDisable2(GL_TEXTURE_2D);
			glDisable2(GL_LIGHTING);
			glDisable2(GL_ALPHA_TEST);
			glDisable2(GL_FOG);

			glColorMask(false, false, false, false);
			glDepthMask(false);
			//checkQueryResults(from, to);
			glPushMatrix2();
			float xo = 0;
			float yo = 0;
			float zo = 0;
			for (int i = from; i < to; i++) {
				if (sortedChunks[i]->isEmpty()) {
					sortedChunks[i]->visible = false;
					continue;
				}
				if (!sortedChunks[i]->visible) {
					sortedChunks[i]->occlusion_visible = true;
				}

				if (sortedChunks[i]->visible && !sortedChunks[i]->occlusion_querying) {
					float dist = Mth::sqrt(sortedChunks[i]->distanceToSqr(player));

					int frequency = (int) (1 + dist / 128);

					if (ticks % frequency == i % frequency) {
						Chunk* chunk = sortedChunks[i];
						float xt = (float) ((chunk->x - Chunk::originX) - xOff);
						float yt = (float) (chunk->y - yOff);
						float zt = (float) ((chunk->z - Chunk::originZ) - zOff);
						float xdd = xt - xo;
						float ydd = yt - yo;
						float zdd = zt - zo;

						if (xdd != 0 || ydd != 0 || zdd != 0) {
							glTranslatef2(xdd, ydd, zdd);
							xo += xdd;
							yo += ydd;
							zo += zdd;
						}

						sortedChunks[i]->renderBB();
						sortedChunks[i]->occlusion_querying = true;
					}
				}
			}
			glPopMatrix2();
			glColorMask(true, true, true, true);
			glDepthMask(true);
			glEnable2(GL_TEXTURE_2D);
			glEnable2(GL_ALPHA_TEST);
			glEnable2(GL_FOG);

			count += renderChunks(from, to, layer, alpha);

		} while (to < chunksLength);

	} else {
		TIMER_POP_PUSH("render");
		count += renderChunks(0, chunksLength, layer, alpha);
	}

	TIMER_POP();
	return count;
}

void LevelRenderer::renderDebug(const AABB& b, float a) const {
	float x0 = b.x0;
	float x1 = b.x1;
	float y0 = b.y0;
	float y1 = b.y1;
	float z0 = b.z0;
	float z1 = b.z1;
	float u0 = 0, v0 = 0;
	float u1 = 1, v1 = 1;

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);
	glDisable2(GL_TEXTURE_2D);
	glColor4f2(1, 1, 1, 1);

	textures->loadAndBindTexture("terrain.png");

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(255, 255, 255, 255);

	t.offset(((Mob*)mc->player)->getPos(a).negated());

	// up
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x0, y0, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u1, v0);
	t.vertexUV(x1, y0, z1, u1, v1);

	// down
	t.vertexUV(x1, y1, z1, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y1, z1, u0, v1);

	// north
	t.vertexUV(x0, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z0, u0, v0);
	t.vertexUV(x1, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z0, u1, v1);

	// south
	t.vertexUV(x0, y1, z1, u0, v0);
	t.vertexUV(x0, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z1, u1, v1);
	t.vertexUV(x1, y1, z1, u1, v0);

	// west
	t.vertexUV(x0, y1, z1, u1, v0);
	t.vertexUV(x0, y1, z0, u0, v0);
	t.vertexUV(x0, y0, z0, u0, v1);
	t.vertexUV(x0, y0, z1, u1, v1);

	// east
	t.vertexUV(x1, y0, z1, u0, v1);
	t.vertexUV(x1, y0, z0, u1, v1);
	t.vertexUV(x1, y1, z0, u1, v0);
	t.vertexUV(x1, y1, z1, u0, v0);

	t.offset(0, 0, 0);
	t.draw();

	glEnable2(GL_TEXTURE_2D);
	glDisable2(GL_BLEND);
}

void LevelRenderer::render(const AABB& b) const
{
	Tesselator& t = Tesselator::instance;

	glColor4f2(1, 1, 1, 1);

	textures->loadAndBindTexture("terrain.png");

	//t.begin();
	t.color(255, 255, 255, 255);

	t.offset(((Mob*)mc->player)->getPos(0).negated());

	t.begin(GL_LINE_STRIP);
	t.vertex(b.x0, b.y0, b.z0);
	t.vertex(b.x1, b.y0, b.z0);
	t.vertex(b.x1, b.y0, b.z1);
	t.vertex(b.x0, b.y0, b.z1);
	t.vertex(b.x0, b.y0, b.z0);
	t.draw();

	t.begin(GL_LINE_STRIP);
	t.vertex(b.x0, b.y1, b.z0);
	t.vertex(b.x1, b.y1, b.z0);
	t.vertex(b.x1, b.y1, b.z1);
	t.vertex(b.x0, b.y1, b.z1);
	t.vertex(b.x0, b.y1, b.z0);
	t.draw();

	t.begin(GL_LINES);
	t.vertex(b.x0, b.y0, b.z0);
	t.vertex(b.x0, b.y1, b.z0);
	t.vertex(b.x1, b.y0, b.z0);
	t.vertex(b.x1, b.y1, b.z0);
	t.vertex(b.x1, b.y0, b.z1);
	t.vertex(b.x1, b.y1, b.z1);
	t.vertex(b.x0, b.y0, b.z1);
	t.vertex(b.x0, b.y1, b.z1);

	t.offset(0, 0, 0);
	t.draw();
}

//void LevelRenderer::checkQueryResults( int from, int to )
//{
//	for (int i = from; i < to; i++) {
//		if (sortedChunks[i]->occlusion_querying) {
//			// I wanna do a fast occusion culler here.
//		}
//	}
//}

int LevelRenderer::renderChunks( int from, int to, int layer, float alpha )
{
	_renderChunks.clear();
	int count = 0;
	for (int i = from; i < to; i++) {
		if (layer == 0) {
			totalChunks++;
			if (sortedChunks[i]->empty[layer]) emptyChunks++;
			else if (!sortedChunks[i]->visible) offscreenChunks++;
			else if (occlusionCheck && !sortedChunks[i]->occlusion_visible) occludedChunks++;
			else renderedChunks++;
		}

		if (!sortedChunks[i]->empty[layer] && sortedChunks[i]->visible && sortedChunks[i]->occlusion_visible) {
			int list = sortedChunks[i]->getList(layer);
			if (list >= 0) {
				_renderChunks.push_back(sortedChunks[i]);
				count++;
			}
		}
	}

	Mob* player = mc->cameraTargetPlayer;
	// Subtracting Chunk::originX/Z here (instead of leaving xOff/zOff as the
	// raw absolute player position) is what keeps this translate small - see
	// the comment on Chunk::originX for why that matters far from spawn.
	float xOff = player->xOld + (player->x - player->xOld) * alpha - Chunk::originX;
	float yOff = player->yOld + (player->y - player->yOld) * alpha;
	float zOff = player->zOld + (player->z - player->zOld) * alpha - Chunk::originZ;

	//int lists = 0;
	renderList.clear();
	renderList.init(xOff, yOff, zOff);

	for (unsigned int i = 0; i < _renderChunks.size(); ++i) {
		Chunk* chunk = _renderChunks[i];
		#ifdef USE_VBO
			renderList.addR(chunk->getRenderChunk(layer));
		#else
			renderList.add(chunk->getList(layer));
		#endif
		renderList.next();
	}

	renderSameAsLast(layer, alpha);

	return count;
}

void LevelRenderer::renderSameAsLast( int layer, float alpha )
{
	renderList.render();
}

void LevelRenderer::tick()
{
	ticks++;
}

bool LevelRenderer::updateDirtyChunks( Mob* player, bool force )
{
	static int lastSortFrame = -1;
	int sortInterval = 6;
	if (dirtyChunks.size() < 256) sortInterval = 2;
	if (dirtyChunks.size() < 96) sortInterval = 1;
		// When sky brightness changes, force-process all sky-lit chunks this frame
		if (_skyChangedPending) {
			force = true;
			_skyChangedPending = false;
		}
	
	// Opportunistic sort to maintain good prioritization without per-frame overhead
	if (ticks - lastSortFrame >= sortInterval && !dirtyChunks.empty()) {
		DirtyChunkSorter dirtyChunkSorter(player);
		std::sort(dirtyChunks.begin(), dirtyChunks.end(), dirtyChunkSorter);
		lastSortFrame = ticks;
	}

#if defined(MACOS) || defined(LINUX) || defined(WIN32)
	// Desktop: Use background threading for chunk tessellation
	
	// First, process chunks that are ready to upload to GPU
	processUploadQueue();
	
	// Queue new chunks for background tessellation
	int visibleQueued = 0;
	int invisibleQueued = 0;
	size_t writeIdx = 0;
	std::vector<Chunk*> toQueue;
	toQueue.reserve(MAX_VISIBLE_REBUILDS_PER_FRAME + MAX_INVISIBLE_REBUILDS_PER_FRAME);

	for (size_t readIdx = 0; readIdx < dirtyChunks.size(); ++readIdx) {
		Chunk* chunk = dirtyChunks[readIdx];

		bool canQueue = false;
		if (force) {
			canQueue = chunk->visible;
		} else if (chunk->visible) {
			canQueue = visibleQueued < MAX_VISIBLE_REBUILDS_PER_FRAME;
		} else {
			canQueue = invisibleQueued < MAX_INVISIBLE_REBUILDS_PER_FRAME;
		}

		if (canQueue && !chunk->_inFlight.load(std::memory_order_acquire)) {
			chunk->_inFlight.store(true, std::memory_order_release);
			toQueue.push_back(chunk);
			if (chunk->visible) visibleQueued++;
			else invisibleQueued++;
			// Don't copy to writeIdx - effectively removing this chunk
		} else {
			// Keep this chunk for next frame
			dirtyChunks[writeIdx++] = chunk;
		}
	}

	if (!toQueue.empty()) {
		{
			std::lock_guard<std::mutex> lock(buildQueueMutex);
			for (size_t i = 0; i < toQueue.size(); ++i)
				buildQueue.push(toQueue[i]);
		}
		if (toQueue.size() > 1)
			_buildWorkCV.notify_all();
		else
			_buildWorkCV.notify_one();
	}
	dirtyChunks.resize(writeIdx);
	
	return dirtyChunks.empty();
#else
	// Mobile/non-threaded path: rebuild on main thread
	int visibleRebuilt = 0;
	int invisibleRebuilt = 0;
	size_t writeIdx = 0;

	for (size_t readIdx = 0; readIdx < dirtyChunks.size(); ++readIdx) {
		Chunk* chunk = dirtyChunks[readIdx];

		bool canRebuild = false;
		if (force) {
			canRebuild = chunk->visible;
		} else if (chunk->visible) {
			canRebuild = visibleRebuilt < MAX_VISIBLE_REBUILDS_PER_FRAME;
		} else {
			canRebuild = invisibleRebuilt < MAX_INVISIBLE_REBUILDS_PER_FRAME;
		}

		if (canRebuild) {
			chunk->rebuild();
			chunk->setClean();
			if (chunk->visible) visibleRebuilt++;
			else invisibleRebuilt++;
			// Don't copy to writeIdx - effectively removing this chunk
		} else {
			// Keep this chunk for next frame - use swap pattern to maintain order
			dirtyChunks[writeIdx++] = chunk;
		}
	}
	dirtyChunks.resize(writeIdx);

	return dirtyChunks.empty();
#endif
}

void LevelRenderer::renderHit( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	if (mode == 0) {
		if (destroyProgress > 0) {
			Tesselator& t = Tesselator::instance;
			glEnable2(GL_BLEND);
			glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);

			textures->loadAndBindTexture("terrain.png");
			glPushMatrix2();

			int tileId = level->getTile(h.x, h.y, h.z);
			Tile* tile = tileId > 0 ? Tile::tiles[tileId] : NULL;
			//glDisable2(GL_ALPHA_TEST);

			glPolygonOffset(-3.0f, -3.0f);
			glEnable2(GL_POLYGON_OFFSET_FILL);
			t.begin();
			t.color(1.0f, 1.0f, 1.0f, 0.5f);
			t.noColor();
			float xo = player->xOld + (player->x - player->xOld) * a;
			float yo = player->yOld + (player->y - player->yOld) * a;
			float zo = player->zOld + (player->z - player->zOld) * a;

			t.offset(-xo, -yo, -zo);
			//t.noColor();

			if (tile == NULL) tile = Tile::rock;
			const int progress = (int) (destroyProgress * 10);
			tileRenderer->tesselateInWorld(tile, h.x, h.y, h.z, 15 * 16 + progress);

			t.draw();
			t.offset(0, 0, 0);
			glPolygonOffset(0.0f, 0.0f);
			glDisable2(GL_POLYGON_OFFSET_FILL);
			//glDisable2(GL_ALPHA_TEST);
			glDisable2(GL_BLEND);

			glDepthMask(true);
			glPopMatrix2();
		}
	}
	//else if (inventoryItem != NULL) {
	//          glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//          float br = ((float) (util.Mth::sin(System.currentTimeMillis() / 100.0f)) * 0.2f + 0.8f);
	//          glColor4f2(br, br, br, ((float) (util.Mth::sin(System.currentTimeMillis() / 200.0f)) * 0.2f + 0.5f));

	//          int id = textures.loadTexture("terrain.png");
	//          glBindTexture2(GL_TEXTURE_2D, id);
	//          int x = h.x;
	//          int y = h.y;
	//          int z = h.z;
	//          if (h.f == 0) y--;
	//          if (h.f == 1) y++;
	//          if (h.f == 2) z--;
	//          if (h.f == 3) z++;
	//          if (h.f == 4) x--;
	//          if (h.f == 5) x++;
	//          /*
	//           * t.begin(); t.noColor(); Tile.tiles[tileType].tesselate(level, x,
	//           * y, z, t); t.end();
	//           */
	//      }
}

void LevelRenderer::renderHitOutline( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	if (mode == 0 && h.type == TILE) {
		glEnable2(GL_BLEND);
		glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f2(0, 0, 0, 0.4f);
		glLineWidth(1.0f);
		glDisable2(GL_TEXTURE_2D);
		glDepthMask(false);
		float ss = 0.002f;
		int tileId = level->getTile(h.x, h.y, h.z);

		if (tileId > 0) {
			Tile::tiles[tileId]->updateShape(level, h.x, h.y, h.z);
			float xo = player->xOld + (player->x - player->xOld) * a;
			float yo = player->yOld + (player->y - player->yOld) * a;
			float zo = player->zOld + (player->z - player->zOld) * a;
			render(Tile::tiles[tileId]->getTileAABB(level, h.x, h.y, h.z).grow(ss, ss, ss).cloneMove(-xo, -yo, -zo));
		}
		glDepthMask(true);
		glEnable2(GL_TEXTURE_2D);
		glDisable2(GL_BLEND);
	}
}

void LevelRenderer::renderPlacementOutline(Player* player, const HitResult& h, float a)
{
	if (h.type != TILE) return;

	float xo = player->xOld + (player->x - player->xOld) * a;
	float yo = player->yOld + (player->y - player->yOld) * a;
	float zo = player->zOld + (player->z - player->zOld) * a;

	// Outline the block being looked at, slightly expanded to sit on top of its surface
	const float ss = 0.003f;
	float x0f = h.x - xo - ss;
	float y0f = h.y - yo - ss;
	float z0f = h.z - zo - ss;
	float x1f = x0f + 1.0f + ss * 2;
	float y1f = y0f + 1.0f + ss * 2;
	float z1f = z0f + 1.0f + ss * 2;

	Tesselator& t = Tesselator::instance;
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable2(GL_TEXTURE_2D);
	glDepthMask(false);
	glLineWidth(1.5f);

	// White outline with slight transparency
	t.begin(GL_LINES);
	t.colorABGR(0xaaffffff);

	// Bottom face
	t.vertex(x0f, y0f, z0f); t.vertex(x1f, y0f, z0f);
	t.vertex(x1f, y0f, z0f); t.vertex(x1f, y0f, z1f);
	t.vertex(x1f, y0f, z1f); t.vertex(x0f, y0f, z1f);
	t.vertex(x0f, y0f, z1f); t.vertex(x0f, y0f, z0f);
	// Top face
	t.vertex(x0f, y1f, z0f); t.vertex(x1f, y1f, z0f);
	t.vertex(x1f, y1f, z0f); t.vertex(x1f, y1f, z1f);
	t.vertex(x1f, y1f, z1f); t.vertex(x0f, y1f, z1f);
	t.vertex(x0f, y1f, z1f); t.vertex(x0f, y1f, z0f);
	// Verticals
	t.vertex(x0f, y0f, z0f); t.vertex(x0f, y1f, z0f);
	t.vertex(x1f, y0f, z0f); t.vertex(x1f, y1f, z0f);
	t.vertex(x1f, y0f, z1f); t.vertex(x1f, y1f, z1f);
	t.vertex(x0f, y0f, z1f); t.vertex(x0f, y1f, z1f);

	t.draw();

	glDepthMask(true);
	glEnable2(GL_TEXTURE_2D);
	glLineWidth(1.0f);
	glDisable2(GL_BLEND);
}

void LevelRenderer::setDirty( int x0, int y0, int z0, int x1, int y1, int z1 )
{
	int _x0 = Mth::intFloorDiv(x0, CHUNK_SIZE);
	int _y0 = Mth::intFloorDiv(y0, CHUNK_SIZE);
	int _z0 = Mth::intFloorDiv(z0, CHUNK_SIZE);
	int _x1 = Mth::intFloorDiv(x1, CHUNK_SIZE);
	int _y1 = Mth::intFloorDiv(y1, CHUNK_SIZE);
	int _z1 = Mth::intFloorDiv(z1, CHUNK_SIZE);

	for (int x = _x0; x <= _x1; x++) {
		int xx = x % xChunks;
		if (xx < 0) xx += xChunks;
		for (int y = _y0; y <= _y1; y++) {
			int yy = y % yChunks;
			if (yy < 0) yy += yChunks;
			for (int z = _z0; z <= _z1; z++) {
				int zz = z % zChunks;
				if (zz < 0) zz += zChunks;

				int p = ((zz) * yChunks + (yy)) * xChunks + (xx);
				Chunk* chunk = chunks[p];
				if (!chunk->isDirty()) {
						chunk->queued = true;
					dirtyChunks.push_back(chunk);
					chunk->setDirty();
				}
			}
		}
	}
}

void LevelRenderer::tileChanged( int x, int y, int z)
{
	setDirty(x - 1, y - 1, z - 1, x + 1, y + 1, z + 1);
}


void LevelRenderer::setTilesDirty( int x0, int y0, int z0, int x1, int y1, int z1 )
{
	setDirty(x0 - 1, y0 - 1, z0 - 1, x1 + 1, y1 + 1, z1 + 1);
}


void LevelRenderer::cull( Culler* culler, float a )
{
	for (int i = 0; i < chunksLength; i++) {
		if (!chunks[i]->isEmpty()) {
			if (!chunks[i]->visible || ((i + cullStep) & 15) == 0) {
				chunks[i]->cull(culler);
			}
		}
	}
	cullStep++;
}

void LevelRenderer::skyColorChanged()
{
	for (int i = 0; i < chunksLength; i++) {
		if (chunks[i]->skyLit) {
			if (!chunks[i]->isDirty()) {
				chunks[i]->queued = true;
				dirtyChunks.push_back(chunks[i]);
				chunks[i]->setDirty();
			}
		}
	}
	_skyChangedPending = true;
}

bool entityRenderPredicate(const Entity* a, const Entity* b) {
	return a->entityRendererId < b->entityRendererId;
}

void LevelRenderer::renderEntities(Vec3 cam, Culler* culler, float a) {
    if (noEntityRenderFrames > 0) {
        noEntityRenderFrames--;
        return;
    }

	TIMER_PUSH("prepare");
    TileEntityRenderDispatcher::getInstance()->prepare(level, textures, mc->font, mc->cameraTargetPlayer, a);
    EntityRenderDispatcher::getInstance()->prepare(level, mc->font, mc->cameraTargetPlayer, &mc->options, a);

    totalEntities = 0;
    renderedEntities = 0;
    culledEntities = 0;

	Entity* player = mc->cameraTargetPlayer;
    EntityRenderDispatcher::xOff = TileEntityRenderDispatcher::xOff = (player->xOld + (player->x - player->xOld) * a);
    EntityRenderDispatcher::yOff = TileEntityRenderDispatcher::yOff = (player->yOld + (player->y - player->yOld) * a);
    EntityRenderDispatcher::zOff = TileEntityRenderDispatcher::zOff = (player->zOld + (player->z - player->zOld) * a);

	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);

	TIMER_POP_PUSH("entities");
	const EntityList& entities = level->getAllEntities();
	totalEntities = entities.size();
	if (totalEntities > 0) {
		Entity** toRender = new Entity*[totalEntities];
		for (int i = 0; i < totalEntities; i++) {
			Entity* entity = entities[i];

				// Expand BB slightly so entities at the frustum edge aren't incorrectly culled
			AABB entityBB = entity->bb.grow(0.5f, 0.5f, 0.5f);
			// entity->bb is absolute world coords; culler's xOff/yOff/zOff are
			// origin-relative (see the comment on Chunk::originX) - shift into
			// the same frame, same as Chunk::cull() does for chunk AABBs.
			// Without this every entity (including the player's own third-
			// person avatar) gets wrongly culled as "not visible" once far
			// enough from spawn that Chunk::originX/Z is nonzero.
			AABB localEntityBB(entityBB.x0 - Chunk::originX, entityBB.y0, entityBB.z0 - Chunk::originZ,
				entityBB.x1 - Chunk::originX, entityBB.y1, entityBB.z1 - Chunk::originZ);
			if (entity->shouldRender(cam) && culler->isVisible(localEntityBB))
			{
				if (entity == mc->cameraTargetPlayer && mc->options.thirdPersonView == 0 && mc->cameraTargetPlayer->isPlayer() && !((Player*)mc->cameraTargetPlayer)->isSleeping()) continue;
				if (entity == mc->cameraTargetPlayer && !mc->options.thirdPersonView)
					continue;
				if (!level->hasChunkAt(Mth::floor(entity->x), Mth::floor(entity->y), Mth::floor(entity->z)))
					continue;

				toRender[renderedEntities++] = entity;
				//EntityRenderDispatcher::getInstance()->render(entity, a);
			}
		}

		if (renderedEntities > 0) {
			std::sort(&toRender[0], &toRender[renderedEntities], entityRenderPredicate);
			for (int i = 0; i < renderedEntities; ++i) {
				EntityRenderDispatcher* disp = EntityRenderDispatcher::getInstance();
				disp->render(toRender[i], a);
			}
		}

		delete[] toRender;
	}

    TIMER_POP_PUSH("tileentities");
    for (unsigned int i = 0; i < level->tileEntities.size(); i++) {
        TileEntityRenderDispatcher::getInstance()->render(level->tileEntities[i], a);
    }

	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);

	TIMER_POP();
}

std::string LevelRenderer::gatherStats1() {
	std::stringstream ss;
	ss << "C: " << renderedChunks << "/" << totalChunks << ". F: " << offscreenChunks << ", O: " << occludedChunks << ", E: " << emptyChunks << "\n";
    return ss.str();
}

//
//    /*public*/ std::string gatherStats2() {
//        return "E: " + renderedEntities + "/" + totalEntities + ". B: " + culledEntities + ", I: " + ((totalEntities - culledEntities) - renderedEntities);
//    }
//
//    int[] toRender = new int[50000];
//    IntBuffer resultBuffer = MemoryTracker.createIntBuffer(64);

void LevelRenderer::renderSky(float alpha) {
    if (mc->level->dimension->foggy) return;

    glDisable2(GL_TEXTURE_2D);
	Vec3 sc = level->getSkyColor(mc->cameraTargetPlayer, alpha);
	float sr = Mth::clamp((float)sc.x, 0.0f, 1.0f);
	float sg = Mth::clamp((float)sc.y, 0.0f, 1.0f);
	float sb = Mth::clamp((float)sc.z, 0.0f, 1.0f);

	if (mc->options.anaglyph3d) {
		float srr = (sr * 30.0f + sg * 59.0f + sb * 11.0f) / 100.0f;
		float sgg = (sr * 30.0f + sg * 70.0f) / (100.0f);
		float sbb = (sr * 30.0f + sb * 70.0f) / (100.0f);

		sr = Mth::clamp(srr, 0.0f, 1.0f);
		sg = Mth::clamp(sgg, 0.0f, 1.0f);
		sb = Mth::clamp(sbb, 0.0f, 1.0f);
	}
	glColor4f2(sr, sg, sb, 1);

    //Tesselator& t = Tesselator::instance;

    glEnable2(GL_FOG);
    glColor4f2(sr, sg, sb, 1.0f);

#if defined(OPENGL_ES) || defined(MACOS) || defined(LINUX)
	drawArrayVT(skyBuffer, skyVertexCount);
#endif
    glEnable2(GL_TEXTURE_2D);
}

void LevelRenderer::renderClouds( float alpha ) {
	//if (!mc->level->dimension->isNaturalDimension()) return;
	glEnable2(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	float yOffs = (float) (mc->player->yOld + (mc->player->y - mc->player->yOld) * alpha);
	int s = 32;
	int d = 256 / s;
	Tesselator& t = Tesselator::instance;

	//glBindTexture(GL_TEXTURE_2D, texturesloadTexture("/environment/clouds.png"));
	textures->loadAndBindTexture("environment/clouds.png");
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Vec3 cc = level->getCloudColor(alpha);
	float cr = (float) cc.x;
	float cg = (float) cc.y;
	float cb = (float) cc.z;

	float scale = 1 / 2048.0f;

	float time = (ticks + alpha);
	float xo = mc->player->xo + (mc->player->x - mc->player->xo) * alpha + time * 0.03f;
	float zo = mc->player->zo + (mc->player->z - mc->player->zo) * alpha;
	int xOffs = Mth::floor(xo / 2048);
	int zOffs = Mth::floor(zo / 2048);
	xo -= xOffs * 2048;
	zo -= zOffs * 2048;

	float yy = /*level.dimension.getCloudHeight()*/ 128 - yOffs + 0.33f;//mc->player->y + 1;
	float uo = (float) (xo * scale);
	float vo = (float) (zo * scale);
	t.begin();

	t.color(cr, cg, cb, 0.8f);
	for (int xx = -s * d; xx < +s * d; xx += s) {
		for (int zz = -s * d; zz < +s * d; zz += s) {
			t.vertexUV((float)xx, yy, (float)zz + s, xx * scale + uo, (zz + s) * scale + vo);
			t.vertexUV((float)xx + s, yy, (float)zz + s, (xx + s) * scale + uo, (zz + s) * scale + vo);
			t.vertexUV((float)xx + s, yy, (float)zz, (xx + s) * scale + uo, zz * scale + vo);
			t.vertexUV((float)xx, yy, (float)zz, xx * scale + uo, zz * scale + vo);
		}
	}
	t.endOverrideAndDraw();
	glColor4f(1, 1, 1, 1.0f);
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}

void LevelRenderer::playSound(const std::string& name, float x, float y, float z, float volume, float pitch) {
	// @todo: deny sounds here if sound is off (rather than waiting 'til SoundEngine)
	float dd = 16;

    if (volume > 1) dd *= volume;
    if (mc->cameraTargetPlayer->distanceToSqr(x, y, z) < dd * dd) {
        mc->soundEngine->play(name, x, y, z, volume, pitch);
    }
}

void LevelRenderer::addParticle(const std::string& name, float x, float y, float z, float xa, float ya, float za, int data) {

	float xd = mc->cameraTargetPlayer->x - x;
    float yd = mc->cameraTargetPlayer->y - y;
    float zd = mc->cameraTargetPlayer->z - z;
	float distanceSquared = xd * xd + yd * yd + zd * zd;

	//Particle* p = NULL;
	//if (name == "hugeexplosion") p = new HugeExplosionSeedParticle(level, x, y, z, xa, ya, za);
	//else if (name == "largeexplode") p = new HugeExplosionParticle(textures, level, x, y, z, xa, ya, za);

	//if (p) {
	//	if (distanceSquared < 32 * 32) {
	//		mc->particleEngine->add(p);
	//	} else { delete p; }
	//	return;
	//}

    const float particleDistance = 16;
    if (distanceSquared > particleDistance * particleDistance) return;

	//static Stopwatch sw;
	//sw.start();

    if (name == "bubble") mc->particleEngine->add(new BubbleParticle(level, x, y, z, xa, ya, za));
	else if (name == "crit") mc->particleEngine->add(new CritParticle2(level, x, y, z, xa, ya, za));
	else if (name == "smoke") mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za));
    //else if (name == "note") mc->particleEngine->add(new NoteParticle(level, x, y, z, xa, ya, za));
    else if (name == "explode") mc->particleEngine->add(new ExplodeParticle(level, x, y, z, xa, ya, za));
    else if (name == "flame") mc->particleEngine->add(new FlameParticle(level, x, y, z, xa, ya, za));
    else if (name == "lava") mc->particleEngine->add(new LavaParticle(level, x, y, z));
    //else if (name == "splash") mc->particleEngine->add(new SplashParticle(level, x, y, z, xa, ya, za));
	else if (name == "largesmoke") mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f));
    else if (name == "reddust") mc->particleEngine->add(new RedDustParticle(level, x, y, z, xa, ya, za));
	else if (name == "iconcrack") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]));
	else if (name == "snowballpoof") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::snowBall));
    //else if (name == "snowballpoof") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::snowBall));
    //else if (name == "slime") mc->particleEngine->add(new BreakingItemParticle(level, x, y, z, Item::slimeBall));
    //else if (name == "heart") mc->particleEngine->add(new HeartParticle(level, x, y, z, xa, ya, za));

	//sw.stop();
	//sw.printEvery(50, "add-particle-string");
}

/*
void LevelRenderer::addParticle(ParticleType::Id name, float x, float y, float z, float xa, float ya, float za, int data) {
	float xd = mc->cameraTargetPlayer->x - x;
	float yd = mc->cameraTargetPlayer->y - y;
	float zd = mc->cameraTargetPlayer->z - z;

	const float particleDistance = 16;
	if (xd * xd + yd * yd + zd * zd > particleDistance * particleDistance) return;

	//static Stopwatch sw;
	//sw.start();

	//Particle* p = NULL;

	if (name == ParticleType::bubble)		mc->particleEngine->add( new BubbleParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::crit)		mc->particleEngine->add(new CritParticle2(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::smoke)		mc->particleEngine->add(new SmokeParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::explode)		mc->particleEngine->add( new ExplodeParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::flame)		mc->particleEngine->add( new FlameParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::lava)		mc->particleEngine->add( new LavaParticle(level, x, y, z) );
	else if (name == ParticleType::largesmoke)	mc->particleEngine->add( new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f) );
	else if (name == ParticleType::reddust)		mc->particleEngine->add( new RedDustParticle(level, x, y, z, xa, ya, za) );
	else if (name == ParticleType::iconcrack)	mc->particleEngine->add( new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]) );

	//switch (name) {
	//	case ParticleType::bubble:		p = new BubbleParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::crit:		p = new CritParticle2(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::smoke:		p = new SmokeParticle(level, x, y, z, xa, ya, za); break;
	//	//case ParticleType::note: p = new NoteParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::explode:		p = new ExplodeParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::flame:		p = new FlameParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::lava:		p = new LavaParticle(level, x, y, z); break;
	//	//case ParticleType::splash: p = new SplashParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::largesmoke:	p = new SmokeParticle(level, x, y, z, xa, ya, za, 2.5f); break;
	//	case ParticleType::reddust:		p = new RedDustParticle(level, x, y, z, xa, ya, za); break;
	//	case ParticleType::iconcrack:	p = new BreakingItemParticle(level, x, y, z, xa, ya, za, Item::items[data]); break;
	//	//case ParticleType::snowballpoof: p = new BreakingItemParticle(level, x, y, z, Item::snowBall); break;
	//	//case ParticleType::slime: p = new BreakingItemParticle(level, x, y, z, Item::slimeBall); break;
	//	//case ParticleType::heart: p = new HeartParticle(level, x, y, z, xa, ya, za); break;
	//	default:
	//		LOGW("Couldn't find particle of type: %d\n", name);
	//		break;
	//}
	//if (p) {
	//	mc->particleEngine->add(p);
	//}

	//sw.stop();
	//sw.printEvery(50, "add-particle-enum");
}
*/

void LevelRenderer::renderHitSelect( Player* player, const HitResult& h, int mode, /*ItemInstance*/void* inventoryItem, float a )
{
	//if (h.type == TILE) LOGI("type: %s @ (%d, %d, %d)\n", Tile::tiles[level->getTile(h.x, h.y, h.z)]->getDescriptionId().c_str(), h.x, h.y, h.z);

	if (mode == 0) {

		Tesselator& t = Tesselator::instance;
		glEnable2(GL_BLEND);
		glDisable2(GL_TEXTURE_2D);
		glBlendFunc2(GL_SRC_ALPHA, GL_ONE);
		glBlendFunc2(GL_DST_COLOR, GL_SRC_COLOR);
		glEnable2(GL_DEPTH_TEST);

		textures->loadAndBindTexture("terrain.png");
		
		int tileId = level->getTile(h.x, h.y, h.z);
		Tile* tile = tileId > 0 ? Tile::tiles[tileId] : NULL;
		glDisable2(GL_ALPHA_TEST);

		//LOGI("block: %d - %d (%s)\n", tileId, level->getData(h.x, h.y, h.z), tile==NULL?"null" : tile->getDescriptionId().c_str() );

		const float br = 0.65f;
		glColor4f2(br * 1.0f, br * 1.0f, br * 1.0f, br * 1.0f);
		glPushMatrix2();

		//glPolygonOffset(-.3f, -.3f);
		glPolygonOffset(-1.f, -1.f); //Implementation dependent units
		glEnable2(GL_POLYGON_OFFSET_FILL);
		float xo = player->xOld + (player->x - player->xOld) * a;
		float yo = player->yOld + (player->y - player->yOld) * a;
		float zo = player->zOld + (player->z - player->zOld) * a;

		t.begin();
		t.offset(-xo, -yo, -zo);
		t.noColor();

		if (tile == NULL) tile = Tile::rock;
		tileRenderer->tesselateInWorld(tile, h.x, h.y, h.z);

		t.draw();
		t.offset(0, 0, 0);
		glPolygonOffset(0.0f, 0.0f);

		glDisable2(GL_POLYGON_OFFSET_FILL);
		glEnable2(GL_TEXTURE_2D);

		glDepthMask(true);
		glPopMatrix2();

		glEnable2(GL_ALPHA_TEST);
		glDisable2(GL_BLEND);
	}
}

void LevelRenderer::onGraphicsReset()
{
#if defined(OPENGL_ES) || defined(MACOS) || defined(LINUX)
	generateSky();
#endif

	// Get new buffers
#ifdef USE_VBO
	glGenBuffers2(numListsOrBuffers, chunkBuffers);
#else
	chunkLists = glGenLists(numListsOrBuffers);
#endif

	// Rebuild
	allChanged();
}

void LevelRenderer::entityAdded( Entity* entity )
{
	if (!entity->isPlayer())
		return;

	// Hack to (hopefully) get the players to show
	EntityRenderDispatcher::getInstance()->onGraphicsReset();
}

int _t_keepPic = -1;

void LevelRenderer::takePicture( TripodCamera* cam, Entity* entity )
{
	// Push old values
	Mob* oldCameraEntity = mc->cameraTargetPlayer;
	bool hideGui = mc->options.hideGui;
	bool thirdPerson = mc->options.thirdPersonView;

	// @huge @attn: This is highly illegal, super temp!
	mc->cameraTargetPlayer = (Mob*)cam;
	mc->options.hideGui = true;
	mc->options.thirdPersonView = false;

	mc->gameRenderer->renderLevel(0);

	// Pop values back
	mc->cameraTargetPlayer = oldCameraEntity;
	mc->options.hideGui = hideGui;
	mc->options.thirdPersonView = thirdPerson;

	_t_keepPic = -1;

	// Save image
	static char filename[256];
	sprintf(filename, "%s/games/com.mojang/img_%.4d.jpg", mc->externalStoragePath.c_str(), getTimeMs());

	mc->platform()->saveScreenshot(filename, mc->width, mc->height);
}

void LevelRenderer::levelEvent(Player* player, int type, int x, int y, int z, int data) {
	switch (type) {    
	case LevelEvent::SOUND_OPEN_DOOR:
        if (Mth::random() < 0.5f) {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.door_open", 1, level->random.nextFloat() * 0.1f + 0.9f);
        } else {
            level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, "random.door_close", 1, level->random.nextFloat() * 0.1f + 0.9f);
        }
        break;
	}
}
