#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION)

#include "LevelData.h"
#include "RegionFile.h"
#include "ExternalFileLevelStorage.h"
#include "FolderMethods.h"
#include "../chunk/LevelChunk.h"
#include "../chunk/ChunkSource.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include "../tile/TreeTile.h"
#include "../../entity/EntityFactory.h"
#include "../../../nbt/NbtIo.h"
#include "../../../util/RakDataIO.h"
#include "../../../raknet/GetTime.h"
#include "../tile/entity/TileEntity.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

static const int ChunkVersion_Light = 1;
static const int ChunkVersion_Entity = 2;

const char* const fnLevelDatOld = "level.dat_old";
const char* const fnLevelDatNew = "level.dat_new";
const char* const fnLevelDat    = "level.dat";
const char* const fnPlayerDat   = "player.dat";
const char* const fnOpsTxt      = "ops.txt";

static std::string normalizePlayerName(const std::string& name)
{
	std::string normalized;
	normalized.reserve(name.size());
	for (size_t i = 0; i < name.size(); ++i)
		normalized += char(std::tolower((unsigned char)name[i]));
	return normalized;
}

static std::string getOpsPath(const std::string& levelPath)
{
	return levelPath + "/" + fnOpsTxt;
}

static std::set<std::string> readOperators(const std::string& levelPath)
{
	std::set<std::string> operators;
	std::ifstream stream(getOpsPath(levelPath).c_str());
	std::string line;
	while (std::getline(stream, line)) {
		line.erase(std::remove_if(line.begin(), line.end(), [](char ch) {
			return ch == '\r' || ch == '\n';
		}), line.end());
		if (!line.empty())
			operators.insert(normalizePlayerName(line));
	}
	return operators;
}

static bool writeOperators(const std::string& levelPath, const std::set<std::string>& operators)
{
	std::ofstream stream(getOpsPath(levelPath).c_str(), std::ios::out | std::ios::trunc);
	if (!stream.is_open())
		return false;
	for (std::set<std::string>::const_iterator it = operators.begin(); it != operators.end(); ++it)
		stream << *it << "\n";
	return true;
}

//
// Helpers for converting old levels to newer
//
class LevelConverters
{
public:
	// Replacing old Cloth (id based) with new Cloth (data based)
	static bool v1_ClothIdToClothData(LevelChunk* c) {
		bool changed = false;
		unsigned char* blocks = c->getBlockData();
		unsigned char newTile = Tile::cloth->id;

		for (int i = 0; i < 16*16*128; ++i) {
			unsigned char oldTile = blocks[i];
			//Tile::cloth_00 to Tile::cloth_61
			if (oldTile >= 101 && oldTile <= 115) {
				int color = 0xf - (oldTile - 101);
				blocks[i] = newTile;
				c->data.set(i, color);
				changed = true;
			}
		}
		return changed;
	}

	// Replacing unavailable blocks with "Update!" blocks
	static bool ReplaceUnavailableBlocks(LevelChunk* c) {
		//int st = getTimeMs();
		
		bool changed = false;
		unsigned char* blocks = c->getBlockData();

		for (int i = 0; i < 16*16*128; ++i) {
			unsigned char oldTile = blocks[i];
			unsigned char newTile = Tile::transformToValidBlockId(oldTile);
			if (oldTile != newTile) {
				blocks[i] = newTile;
				changed = true;
			}
		}

		//int et = getTimeMs();
		//LOGI("time: %d\n", et - st);
		return changed;
	}

	//static bool ConvertPlayerDatToLevelDat() {
	//	return false;
	//}
};


ExternalFileLevelStorage::ExternalFileLevelStorage(const std::string& levelId, const std::string& fullPath)
:	levelId(levelId),
	levelPath(fullPath),
	loadedLevelData(NULL),
	regionFile(NULL),
	entitiesFile(NULL),
	tickCount(0),
	lastSavedEntitiesTick(-999999),
	level(NULL),
	loadedStorageVersion(SharedConstants::StorageVersion)
{
	createFolderIfNotExists(levelPath.c_str());

	std::string datFileName   = levelPath + "/" + fnLevelDat;
	std::string levelFileName = levelPath + "/" + fnPlayerDat;
	loadedLevelData = new LevelData();
	if (readLevelData(levelPath, *loadedLevelData))
	{
		loadedStorageVersion = loadedLevelData->getStorageVersion();
		readPlayerData(levelFileName, *loadedLevelData);
	} else {
		delete loadedLevelData;
		loadedLevelData = NULL;
	}
}

ExternalFileLevelStorage::~ExternalFileLevelStorage()
{
	delete regionFile;
	delete loadedLevelData;
	for (auto& kv : regionFiles)
		delete kv.second;
}

void ExternalFileLevelStorage::saveLevelData(LevelData& levelData, std::vector<Player*>* players) {
	ExternalFileLevelStorage::saveLevelData(levelPath, levelData, players);
}

void ExternalFileLevelStorage::saveLevelData( const std::string& levelPath, LevelData& levelData, std::vector<Player*>* players )
{
	std::string directory = levelPath + "/";
    std::string tmpFile = directory + fnLevelDatNew;
    std::string datFile = directory + fnLevelDat;
    std::string oldFile = directory + fnLevelDatOld;

	levelData.setStorageVersion(SharedConstants::StorageVersion);
	if (!writeLevelData(tmpFile, levelData, players))
        return;

    // Remove old backup
    remove(oldFile.c_str());

    // If it exists, move the previous save to backup (and possibly delete it)
    if (exists(datFile.c_str())) {
        if (rename(datFile.c_str(), oldFile.c_str())) {
            LOGE("Error@saveLevelData: Couldn't move savefile to level.dat_old\n");
            return;
        }
        remove(datFile.c_str());
    }
    // Move the new save to level.dat
    if (rename(tmpFile.c_str(), datFile.c_str())) {
        LOGE("Error@saveLevelData: Couldn't move new file to level.dat\n");
        return;
    }

    // Remove the temporary save, if the rename didn't do it
    remove(tmpFile.c_str());
}

LevelData* ExternalFileLevelStorage::prepareLevel(Level* _level)
{
	level = _level;
	return loadedLevelData;
}

bool ExternalFileLevelStorage::readLevelData(const std::string& directory, LevelData& levelData)
{
    // Try to load level.dat
    std::string datFilename = directory + "/" + fnLevelDat;
	FILE* file = fopen(datFilename.c_str(), "rb");

    // If that fails, try to load level.dat_old
    if (!file) {
        datFilename = directory + "/" + fnLevelDatOld;
        file = fopen(datFilename.c_str(), "rb");
    }

    if (!file)
        return false;

	int version = 0;
	int size = 0;
	unsigned char* data = NULL;

	do {
		if (fread(&version, sizeof(version), 1, file) != 1)
		{
			break;
		}
		if (fread(&size, sizeof(size), 1, file) != 1)
		{
			break;
		}

		int left = getRemainingFileSize(file);
		if (size > left || size <= 0)
			break;

		data = new unsigned char[size];
		if (fread(data, 1, size, file) != size)
		{
			break;
		}

		if (version == 1) {
			RakNet::BitStream bitStream(data, size, false);
			levelData.v1_read(bitStream, version);
		} else if (version >= 2) {
			//LOGI("---> Trying to load level with version %d\n", version);
			RakNet::BitStream bitStream(data, size, false);
			RakDataInput stream(bitStream);
			//LOGI("dat: %s\n", datFileName.c_str());
			CompoundTag* tag = NbtIo::read(&stream);
			if (tag) {
				levelData.getTagData(tag);
				tag->deleteChildren();
				delete tag;
			}
			//LOGI("<--- Finished reading level tag: %p\n", tag);
		}
	} while (false);

	fclose(file);
	delete [] data;

    return true;
}

bool ExternalFileLevelStorage::writeLevelData(const std::string& datFileName, LevelData& levelData, const std::vector<Player*>* players)
{
	// LOGI("Writing down level seed as: %ld\n", levelData.getSeed());
	//return true;

	// Write level info
	FILE* file = fopen(datFileName.c_str(), "wb");
	if (!file)
        return false;

    //if (levelData.getStorageVersion() == 1) {
	RakNet::BitStream data;
	if (levelData.getStorageVersion() == 1)
		levelData.v1_write(data);
	else {
		RakDataOutput buf(data);
		//LOGI("---> Trying to write level with version %d\n", version);
		CompoundTag* tag = NULL;
		if (players && !players->empty())
			tag = levelData.createTag(*players);
		else
			tag = levelData.createTag();

		NbtIo::write(tag, &buf);
		tag->deleteChildren();
		delete tag;
		//LOGI("<--- Finished writing level data. Size: %d\n", fdout.bytesWritten);
	}

	int version = levelData.getStorageVersion(); // 1
	fwrite(&version, sizeof(version), 1, file);
	int size = data.GetNumberOfBytesUsed();
	fwrite(&size, sizeof(size), 1, file);

	fwrite(data.GetData(), 1, size, file);
	fclose(file);

	return true;
}

bool ExternalFileLevelStorage::readPlayerData(const std::string& filename, LevelData& dest)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return false;

	do {
		int version;
		if (fread(&version, 4, 1, fp) != 1)
			break;

		int size;
		if (fread(&size, 4, 1, fp) != 1)
			break;

		if (version == 1) {
			if (fread(&dest.playerData, 1, sizeof(dest.playerData), fp) != size)
				break;

			// Fix coordinates — only clamp x/z for finite (Old) worlds.
			// Infinite worlds have no boundaries, so don't snap the player.
			Vec3& pos = dest.playerData.pos;
			if (!dest.isInfinite()) {
				if (pos.x < 0.5f) pos.x = 0.5f;
				if (pos.z < 0.5f) pos.z = 0.5f;
				if (pos.x > (LEVEL_WIDTH - 0.5f)) pos.x = LEVEL_WIDTH - 0.5f;
				if (pos.z > (LEVEL_DEPTH - 0.5f)) pos.z = LEVEL_DEPTH - 0.5f;
			}
			if (pos.y < 0) pos.y = 64;

			dest.playerDataVersion = version;
		}
	} while (false);

	fclose(fp);
	return true;
}

// Sanitize a player name so it's safe to use as a filename component.
// Only [a-zA-Z0-9_-] are kept; everything else becomes '_'.
static std::string sanitizePlayerName(const std::string& name) {
	std::string safe;
	safe.reserve(name.size());
	for (char c : name) {
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		    (c >= '0' && c <= '9') || c == '_' || c == '-') {
			safe += c;
		} else {
			safe += '_';
		}
	}
	if (safe.empty()) safe = "_unknown_";
	return safe;
}

static std::string getCanonicalPlayerDataPath(const std::string& levelPath, const std::string& playerName) {
	return levelPath + "/players/" + sanitizePlayerName(normalizePlayerName(playerName)) + ".dat";
}

static std::string getLegacyPlayerDataPath(const std::string& levelPath, const std::string& playerName) {
	return levelPath + "/players/" + sanitizePlayerName(playerName) + ".dat";
}

// Account UUIDs (see Player::uuid) never change even if a player renames
// their account, so prefer keying player-data files by uuid once one is
// known. Falls back to the name-keyed path for unauthenticated/local
// sessions, which never have a uuid.
static std::string getUuidPlayerDataPath(const std::string& levelPath, const std::string& accountUuid) {
	return levelPath + "/players/" + sanitizePlayerName(accountUuid) + ".dat";
}

static std::string getPlayerDataPath(const std::string& levelPath, Player* player) {
	if (!player->uuid.empty())
		return getUuidPlayerDataPath(levelPath, player->uuid);
	return getCanonicalPlayerDataPath(levelPath, player->name);
}

void ExternalFileLevelStorage::saveServerPlayer(Player* player)
{
	const std::string dir = levelPath + "/players/";
	createFolderIfNotExists(dir.c_str());
	const std::string path = getPlayerDataPath(levelPath, player);

	CompoundTag* tag = new CompoundTag();
	player->saveWithoutId(tag);

	RakNet::BitStream bs;
	RakDataOutput out(bs);
	NbtIo::write(tag, &out);
	tag->deleteChildren();
	delete tag;

	FILE* f = fopen(path.c_str(), "wb");
	if (f) {
		fwrite(bs.GetData(), 1, bs.GetNumberOfBytesUsed(), f);
		fclose(f);
		LOGI("Saved player data for: %s\n", player->name.c_str());
	} else {
		LOGE("Failed to save player data for: %s\n", player->name.c_str());
	}
}

bool ExternalFileLevelStorage::loadServerPlayer(Player* player)
{
	const std::string canonicalPath = getPlayerDataPath(levelPath, player);
	FILE* f = fopen(canonicalPath.c_str(), "rb");
	if (!f && !player->uuid.empty()) {
		// This account has a uuid but no uuid-keyed save yet — it may be a
		// returning player from before uuids existed (or before this account
		// was ever contacted for one). Fall back to their old name-keyed file
		// so they don't lose progress; saveServerPlayer() will write the
		// uuid-keyed file from now on, migrating them automatically.
		const std::string legacyByName = getCanonicalPlayerDataPath(levelPath, player->name);
		f = fopen(legacyByName.c_str(), "rb");
	}
	if (!f) {
		// Backward compatibility with older mixed-case filename scheme.
		const std::string legacyPath = getLegacyPlayerDataPath(levelPath, player->name);
		f = fopen(legacyPath.c_str(), "rb");
	}
	if (!f) return false;  // First connection — use defaults

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size <= 0) { fclose(f); return false; }

	unsigned char* data = new unsigned char[size];
	bool ok = ((long)fread(data, 1, size, f) == size);
	fclose(f);

	if (ok) {
		RakNet::BitStream bs(data, size, false);
		RakDataInput in(bs);
		CompoundTag* tag = NbtIo::read(&in);
		if (tag) {
			player->load(tag);
			tag->deleteChildren();
			delete tag;
			LOGI("Loaded player data for: %s\n", player->name.c_str());
		}
	}
	delete[] data;
	return ok;
}

bool ExternalFileLevelStorage::hasOperators() const
{
	return !readOperators(levelPath).empty();
}

bool ExternalFileLevelStorage::isOperator(const std::string& name) const
{
	std::set<std::string> operators = readOperators(levelPath);
	return operators.find(normalizePlayerName(name)) != operators.end();
}

bool ExternalFileLevelStorage::setOperator(const std::string& name, bool isOp)
{
	std::set<std::string> operators = readOperators(levelPath);
	const std::string normalized = normalizePlayerName(name);
	if (normalized.empty())
		return false;
	if (isOp)
		operators.insert(normalized);
	else
		operators.erase(normalized);
	return writeOperators(levelPath, operators);
}

void ExternalFileLevelStorage::tick()
{
	tickCount++;
	if ((tickCount % 50) == 0 && level)
	{
		// Save unsaved chunks from the active chunk source (works for both
		// finite and infinite worlds, including negative chunk coordinates).
		ChunkSource* chunkSource = level->getChunkSource();
		if (chunkSource)
			chunkSource->saveAll(true);

		// Also flush any pending legacy queue entries.
		savePendingUnsavedChunks(2);
	}
	if (tickCount - lastSavedEntitiesTick > (60 * SharedConstants::TicksPerSecond)) {
		saveEntities(level, NULL);
	}
}

// Floor-divide cx by 32, rounding toward -infinity (for negative chunk coords).
static inline int floorDivBy32(int v)
{
	return v < 0 ? (v - 31) / 32 : v / 32;
}

static inline int64_t regionKey(int rx, int rz)
{
	return ((int64_t)(uint32_t)rx << 32) | (uint32_t)rz;
}

RegionFile* ExternalFileLevelStorage::getOrOpenRegion(int cx, int cz)
{
	int rx = floorDivBy32(cx);
	int rz = floorDivBy32(cz);
	int64_t key = regionKey(rx, rz);

	auto it = regionFiles.find(key);
	if (it != regionFiles.end())
		return it->second; // may be nullptr if open() failed previously

	std::string path = levelPath + "/chunks.r." +
	                   std::to_string(rx) + "." + std::to_string(rz) + ".dat";
	RegionFile* rf = new RegionFile(path, true);
	if (!rf->open())
	{
		delete rf;
		rf = NULL;
	}
	regionFiles[key] = rf;
	return rf;
}

void ExternalFileLevelStorage::save(Level* level, LevelChunk* levelChunk)
{
	int cx = levelChunk->x;
	int cz = levelChunk->z;
	int lx = cx, lz = cz; // local coords within the region file

	RegionFile* rf;
	if (level->isInfinite())
	{
		rf = getOrOpenRegion(cx, cz);
		if (!rf) return;
		int rx = floorDivBy32(cx);
		int rz = floorDivBy32(cz);
		lx = cx - rx * 32;
		lz = cz - rz * 32;
	}
	else
	{
		if (!regionFile)
		{
			regionFile = new RegionFile(levelPath);
			if (!regionFile->open())
			{
				delete regionFile;
				regionFile = NULL;
				return;
			}
		}
		rf = regionFile;
	}

	// Write chunk
	RakNet::BitStream chunkData;
	chunkData.Write((const char*)levelChunk->getBlockData(), CHUNK_BLOCK_COUNT);
	chunkData.Write((const char*)levelChunk->data.data, CHUNK_BLOCK_COUNT / 2);

	chunkData.Write((const char*)levelChunk->skyLight.data, CHUNK_BLOCK_COUNT / 2);
	chunkData.Write((const char*)levelChunk->blockLight.data, CHUNK_BLOCK_COUNT / 2);

	chunkData.Write((const char*)levelChunk->updateMap, CHUNK_COLUMNS);

	if (rf->writeChunk(lx, lz, chunkData))
		levelChunk->unsaved = false; // cleared so shouldSave() returns false until the next block change

	//LOGI("Saved chunk (%d, %d)\n", levelChunk->x, levelChunk->z);
}

LevelChunk* ExternalFileLevelStorage::load(Level* level, int x, int z)
{
	int lx = x, lz = z; // local coords within the region file

	RegionFile* rf;
	if (level->isInfinite())
	{
		rf = getOrOpenRegion(x, z);
		if (!rf) return NULL;
		int rx = floorDivBy32(x);
		int rz = floorDivBy32(z);
		lx = x - rx * 32;
		lz = z - rz * 32;
	}
	else
	{
		if (!regionFile)
		{
			regionFile = new RegionFile(levelPath);
			if (!regionFile->open())
			{
				delete regionFile;
				regionFile = NULL;
				return NULL;
			}
			// Pre-read the entire region file into memory so all subsequent
			// readChunk() calls are pure memory accesses with no fseek/fread.
			// Call finishPreload() once the chunk load loop is complete.
			regionFile->preloadToMemory();
		}
		rf = regionFile;
	}

	RakNet::BitStream* chunkData = NULL;
	if (!rf->readChunk(lx, lz, &chunkData))
	{
		//LOGI("Failed to read data for %d, %d\n", x, z);
		return NULL;
	}

	chunkData->ResetReadPointer();

	unsigned char* blockIds = new unsigned char[CHUNK_BLOCK_COUNT];
	chunkData->Read((char*)blockIds, CHUNK_BLOCK_COUNT);

	LevelChunk* levelChunk = new LevelChunk(level, blockIds, x, z);
	chunkData->Read((char*)levelChunk->data.data, CHUNK_BLOCK_COUNT / 2);
	if (loadedStorageVersion >= ChunkVersion_Light) {
		chunkData->Read((char*)levelChunk->skyLight.data, CHUNK_BLOCK_COUNT / 2);
		chunkData->Read((char*)levelChunk->blockLight.data, CHUNK_BLOCK_COUNT / 2);
	}
	chunkData->Read((char*)levelChunk->updateMap, CHUNK_COLUMNS);
	// This will be difficult to maintain.. Storage version could be per chunk
	// too (but probably better to just read all -> write all, so that all
	// chunks got same version anyway)
	//if (loadedStorageVersion >= ChunkVersion_Entity) {
	//	int dictSize;
	//	chunkData->Read(dictSize);

	//	RakDataInput dis(*chunkData);
	//	Tag* tmp = Tag::readNamedTag(&dis);
	//	if (tmp && tmp->getId() == Tag::TAG_Compound) {
	//		CompoundTag* tag = (CompoundTag*) tmp;

	//		delete tmp;
	//	}
	//}

	delete [] chunkData->GetData();
	delete chunkData;

	//bool dbg = (x == 7 && z == 9);

	//int t = 0;
	//for (int i = 0; i < CHUNK_COLUMNS; ++i) {
	//	char bits = levelChunk->updateMap[i];
	//	t += (bits != 0);
	//	int xx = x * 16 + i%16;
	//	int zz = z * 16 + i/16;
	//	if (dbg && xx == 125 && zz == 152) {
	//		LOGI("xz: %d, %d: %d\n", xx, zz, bits);
	//		for (int j = 0; j < 8; ++j) {
	//			if (bits & (1 << j)) {
	//				LOGI("%d - %d\n", j << 4, ((j+1) << 4) - 1);
	//			}
	//		}
	//	}
	//}

	//
	// Convert LevelChunks here if necessary
	//
	//LOGI("level version: %d: upd: %d - (%d, %d)\n", loadedStorageVersion, t, x, z);
	bool changed = false;

	// Loaded level has old Cloth types (one Tile* per color)
	if (loadedStorageVersion == 1)
		changed |= LevelConverters::v1_ClothIdToClothData(levelChunk);

	// Loaded level is newer than our level - replace all unavailable block types
	//if (loadedStorageVersion > SharedConstants::StorageVersion)
		changed |= LevelConverters::ReplaceUnavailableBlocks(levelChunk);

	levelChunk->recalcHeightmap();
	levelChunk->unsaved = changed;
	levelChunk->terrainPopulated = true;
	levelChunk->createdFromSave = true;

	return levelChunk;
}

void ExternalFileLevelStorage::saveEntities( Level* level, LevelChunk* levelChunk )
{
	lastSavedEntitiesTick = tickCount;
	int count = 0;
	float st = getTimeS();

	// Version 1: Save ALL Entities for all chunks in one structure
	EntityList& entities = level->entities;

	ListTag* entityTags = new ListTag();
	for (unsigned int i = 0; i < entities.size(); ++i) {
		Entity* e = entities[i];

		CompoundTag* tag = new CompoundTag();
		if (e->save(tag)) {
			count++;
			entityTags->add(tag);
		} else
			delete tag;
	}

	// Version 1: Save ALL TileEntities for all chunks in one structure
	TileEntityList& tileEntities = level->tileEntities;
	//TileEntityList keep, dontKeep;
	//partitionTileEntities

	ListTag* tileEntityTags = new ListTag();
	for (unsigned int i = 0; i < tileEntities.size(); ++i) {
		TileEntity* e = tileEntities[i];
		if (!e->shouldSave()) continue;

		CompoundTag* tag = new CompoundTag();
		if (e->save(tag)) {
			count++;
			tileEntityTags->add(tag);
		} else
			delete tag;
	}

	CompoundTag base;
	base.put("Entities", entityTags);
	base.put("TileEntities", tileEntityTags);

	RakNet::BitStream stream;
	RakDataOutput dos(stream);
	NbtIo::write(&base, &dos);
	int numBytes = stream.GetNumberOfBytesUsed();

	FILE* fp = fopen((levelPath + "/entities.dat").c_str(), "wb");
	if (fp) {
		int version = 1;
		fwrite("ENT\0", 1, 4, fp);
		fwrite(&version, sizeof(int), 1, fp);
		fwrite(&numBytes, sizeof(int), 1, fp);
		fwrite(stream.GetData(), 1, numBytes, fp);
		fclose(fp);
	}

	base.deleteChildren();

	float tt = getTimeS() - st;
	LOGI("Time to save %d entities: %f s. Size: %d bytes\n", count, tt, numBytes);
}

void ExternalFileLevelStorage::loadEntities(Level* level, LevelChunk* chunk) {
	lastSavedEntitiesTick = tickCount;
	FILE* fp = fopen((levelPath + "/entities.dat").c_str(), "rb");
	if (fp) {
		char header[5];
		int version, numBytes;
		fread(header, 1, 4, fp);
		fread(&version, sizeof(int), 1, fp);
		fread(&numBytes, sizeof(int), 1, fp);

		int left = getRemainingFileSize(fp);
		if (numBytes <= left && numBytes > 0) {
			unsigned char* buf = new unsigned char[numBytes];

			fread(buf, 1, numBytes, fp);

			RakNet::BitStream stream(buf, numBytes, false);
			RakDataInput dis(stream);

			CompoundTag* tag = NbtIo::read(&dis);
			//
			// Read Entity:es
			//
			if (tag->contains("Entities", Tag::TAG_List)) {
				ListTag* entityTags = tag->getList("Entities");
				for (int i = 0; i < entityTags->size(); ++i) {
					Tag* _et = entityTags->get(i);
					if (!_et || _et->getId() != Tag::TAG_Compound) {
						LOGE("Entity tag is either NULL or not a compoundTag: %p : %d!\n", _et, _et?_et->getId() : -1);
						continue;
					}
					CompoundTag* et = (CompoundTag*)_et;
					if (Entity* e = EntityFactory::loadEntity(et, level)) {
						level->addEntity(e);
					}
				}
			}
			//
			// Read TileEntity:s
			//
			if (tag->contains("TileEntities", Tag::TAG_List)) {
				ListTag* tileEntityTags = tag->getList("TileEntities");
				for (int i = 0; i < tileEntityTags->size(); ++i) {
					Tag* _et = tileEntityTags->get(i);
					if (!_et || _et->getId() != Tag::TAG_Compound) {
						LOGE("TileEntity tag is either NULL or not a compoundTag: %p : %d!\n", _et, _et?_et->getId() : -1);
						continue;
					}
					CompoundTag* et = (CompoundTag*)_et;
					if (TileEntity* e = TileEntity::loadStatic(et)) {
						
						LevelChunk* chunk = level->getChunkAt(e->x, e->z);
						if (chunk && !chunk->hasTileEntityAt(e)) {
							LOGI("Adding TileEntity %d to %d, %d, %d\n", e->type, e->x, e->y, e->z);
							chunk->addTileEntity(e);
						} else {
							if (!chunk)
								LOGE("Couldn't find chunk at %d, %d to add %d\n", e->x, e->z, e->type);
							else
								LOGE("Already have TileEntity at %d, %d to add %d\n", e->x, e->z, e->type);
							delete e;
						}
					}
				}
			}

			tag->deleteChildren();
			delete tag;

			delete[] buf;
		}
		LOGI("header: %s, version: %d, bytes: %d (remaining: %d)\n", header, version, numBytes, left);

		//fread(stream.GetData(), 1, numBytes, fp);
		fclose(fp);
	}
}

void ExternalFileLevelStorage::saveGame(Level* level) {
	saveEntities(level, NULL);
}

int ExternalFileLevelStorage::savePendingUnsavedChunks( int maxCount ) {
    if (maxCount < 0)
        maxCount = unsavedChunkList.size();

    int count = 0;
    while (++count <= maxCount && !unsavedChunkList.empty()) {

        UnsavedChunkList::iterator it = unsavedChunkList.begin();
        UnsavedChunkList::iterator remove = unsavedChunkList.begin();
        UnsavedLevelChunk* oldest = &(*it);

        for ( ; it != unsavedChunkList.end(); ++it) {
            if ((*it).addedToList < oldest->addedToList) {
                oldest = &(*it);
                remove = it;
            }
        }
        LevelChunk* chunk = oldest->chunk;
        unsavedChunkList.erase(remove);

        save(level, chunk);
    }
    return count;
}

void ExternalFileLevelStorage::saveAll( Level* level, std::vector<LevelChunk*>& levelChunks ) {
    ChunkStorage::saveAll(level, levelChunks);
	int numChunks = savePendingUnsavedChunks(-1);
}

void ExternalFileLevelStorage::finishPreload()
{
	if (regionFile)
		regionFile->freeMemoryCache();
	for (auto& kv : regionFiles)
		if (kv.second) kv.second->freeMemoryCache();
}

#endif /*DEMO_MODE*/
