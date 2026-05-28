#include "SoundEngine.h"
#include "../Options.h"
#include "../Minecraft.h"
#include "../../world/entity/Mob.h"
#include "../../platform/time.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

SoundEngine::SoundEngine( float maxDistance )
:	idCounter(0),
	mc(0),
	_x(0),
	_y(0),
	_z(0),
	_yRot(0),
	_invMaxDistance(1.0f / maxDistance)
#if !defined(ANDROID) && !defined(RPI)
    ,_musicTrackIdx(0)
    ,_musicWasInWorld(false)
    ,_musicCooldownEndMs(0)
#endif
{

}

SoundEngine::~SoundEngine()
{

}

void SoundEngine::init( Minecraft* mc, Options* options )
{
	this->mc = mc;
	this->options = options;

	if (/*!loaded && */(options == NULL || (options->sound != 0 || options->music != 0))) {
		loadLibrary();
	}

#if !defined(PRE_ANDROID23) && (!defined(__APPLE__) || defined(MACOS)) && !defined(RPI)
	sounds.add("step.cloth", SA_cloth1);
	sounds.add("step.cloth", SA_cloth2);
	sounds.add("step.cloth", SA_cloth3);
	sounds.add("step.cloth", SA_cloth4);
	sounds.add("step.grass", SA_grass1);
	sounds.add("step.grass", SA_grass2);
	sounds.add("step.grass", SA_grass3);
	sounds.add("step.grass", SA_grass4);
	sounds.add("step.gravel", SA_gravel1);
	sounds.add("step.gravel", SA_gravel2);
	sounds.add("step.gravel", SA_gravel3);
	sounds.add("step.gravel", SA_gravel4);
	sounds.add("step.sand", SA_sand1);
	sounds.add("step.sand", SA_sand2);
	sounds.add("step.sand", SA_sand3);
	sounds.add("step.sand", SA_sand4);
	sounds.add("step.stone", SA_stone1);
	sounds.add("step.stone", SA_stone2);
	sounds.add("step.stone", SA_stone3);
	sounds.add("step.stone", SA_stone4);
	sounds.add("step.wood", SA_wood1);
	sounds.add("step.wood", SA_wood2);
	sounds.add("step.wood", SA_wood3);
	sounds.add("step.wood", SA_wood4);

	sounds.add("random.splash", SA_splash);
	sounds.add("random.explode", SA_explode);
	sounds.add("random.click", SA_click);

	sounds.add("random.door_open", SA_door_open);
	sounds.add("random.door_close", SA_door_close);
	sounds.add("random.pop", SA_pop);
	sounds.add("random.pop2", SA_pop2);
	sounds.add("random.hurt", SA_hurt);
	sounds.add("random.glass", SA_glass1);
	sounds.add("random.glass", SA_glass2);
	sounds.add("random.glass", SA_glass3);

	sounds.add("mob.sheep", SA_sheep1);
	sounds.add("mob.sheep", SA_sheep2);
	sounds.add("mob.sheep", SA_sheep3);
	sounds.add("mob.pig", SA_pig1);
	sounds.add("mob.pig", SA_pig2);
	sounds.add("mob.pig", SA_pig3);
	sounds.add("mob.pigdeath", SA_pigdeath);

	sounds.add("mob.cow", SA_cow1);
	sounds.add("mob.cow", SA_cow2);
	sounds.add("mob.cow", SA_cow3);
	sounds.add("mob.cow", SA_cow4);
	sounds.add("mob.cowhurt", SA_cowhurt1);
	sounds.add("mob.cowhurt", SA_cowhurt2);
	sounds.add("mob.cowhurt", SA_cowhurt3);

	sounds.add("mob.chicken", SA_chicken2);
	sounds.add("mob.chicken", SA_chicken3);
	sounds.add("mob.chickenhurt", SA_chickenhurt1);
	sounds.add("mob.chickenhurt", SA_chickenhurt2);

	sounds.add("mob.zombie", SA_zombie1);
	sounds.add("mob.zombie", SA_zombie2);
	sounds.add("mob.zombie", SA_zombie3); 
	sounds.add("mob.zombiedeath", SA_zombiedeath);
	sounds.add("mob.zombiehurt", SA_zombiehurt1);
	sounds.add("mob.zombiehurt", SA_zombiehurt2);

	sounds.add("mob.skeleton", SA_skeleton1);
	sounds.add("mob.skeleton", SA_skeleton2);
	sounds.add("mob.skeleton", SA_skeleton3);
	sounds.add("mob.skeletonhurt", SA_skeletonhurt1);
	sounds.add("mob.skeletonhurt", SA_skeletonhurt2);
	sounds.add("mob.skeletonhurt", SA_skeletonhurt3);
	sounds.add("mob.skeletonhurt", SA_skeletonhurt4);

	sounds.add("mob.spider", SA_spider1);
	sounds.add("mob.spider", SA_spider2);
	sounds.add("mob.spider", SA_spider3);
	sounds.add("mob.spider", SA_spider4);
	sounds.add("mob.spiderdeath", SA_spiderdeath);

	sounds.add("mob.zombiepig.zpig", SA_zpig1);
	sounds.add("mob.zombiepig.zpig", SA_zpig2);
	sounds.add("mob.zombiepig.zpig", SA_zpig3);
	sounds.add("mob.zombiepig.zpig", SA_zpig4);
	sounds.add("mob.zombiepig.zpigangry", SA_zpigangry1);
	sounds.add("mob.zombiepig.zpigangry", SA_zpigangry2);
	sounds.add("mob.zombiepig.zpigangry", SA_zpigangry3);
	sounds.add("mob.zombiepig.zpigangry", SA_zpigangry4);
	sounds.add("mob.zombiepig.zpigdeath", SA_zpigdeath);
	sounds.add("mob.zombiepig.zpighurt", SA_zpighurt1);
	sounds.add("mob.zombiepig.zpighurt", SA_zpighurt2);

	sounds.add("damage.fallbig", SA_fallbig1);
	sounds.add("damage.fallbig", SA_fallbig2);
	sounds.add("damage.fallsmall", SA_fallsmall);

	sounds.add("random.bow", SA_bow);
	sounds.add("random.bowhit", SA_bowhit1);
	sounds.add("random.bowhit", SA_bowhit2);
	sounds.add("random.bowhit", SA_bowhit3);
	sounds.add("random.bowhit", SA_bowhit4);

	sounds.add("mob.creeper", SA_creeper1);
	sounds.add("mob.creeper", SA_creeper2);
	sounds.add("mob.creeper", SA_creeper3);
	sounds.add("mob.creeper", SA_creeper4);
	sounds.add("mob.creeperdeath", SA_creeperdeath);
	sounds.add("random.eat", SA_eat1);
	sounds.add("random.eat", SA_eat2);
	sounds.add("random.eat", SA_eat3);
	sounds.add("random.fuse", SA_fuse);

#endif

#if !defined(ANDROID) && !defined(RPI)
    _initMusic();
#endif
}

void SoundEngine::enable( bool status )
{
#if defined(__APPLE__)
	soundSystem.enable(status);
#endif
}

void SoundEngine::updateOptions()
{

}

void SoundEngine::destroy()
{
	//if (loaded) soundSystem.cleanup();
}

void SoundEngine::update( Mob* player, float a )
{
#if !defined(ANDROID) && !defined(RPI)
    _tickMusic(player != NULL);
#endif

	if (/*!loaded || */options->sound == 0) return;
	if (player == NULL) return;

	_x = player->xo + (player->x - player->xo) * a;
	_y = player->yo + (player->y - player->yo) * a;
	_z = player->zo + (player->z - player->zo) * a;
	_yRot = player->yRotO + (player->yRot - player->yRotO) * a;

	soundSystem.setListenerAngle(_yRot);
	//soundSystem.setListenerPos(_x, _y, _z); //@note: not used, since we translate all sounds to Player space
}

float SoundEngine::_getVolumeMult( float x, float y, float z )
{
	const float dx = x - _x;
	const float dy = y - _y;
	const float dz = z - _z;
	const float dist = Mth::sqrt(dx*dx + dy*dy + dz*dz);
	const float out =  Mth::clamp(1.1f - dist*_invMaxDistance, -1.0f, 1.0f);
	return out;
}

#if defined(PRE_ANDROID23)
void SoundEngine::play(const std::string& name, float x, float y, float z, float volume, float pitch) {
	//volume *= (2.0f * _getVolumeMult(x, y, z))
	if ((volume *= options->sound) <= 0) return;

	volume *= _getVolumeMult(x, y, z);
	mc->platform()->playSound(name, volume, pitch);
}
void SoundEngine::playUI(const std::string& name, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	//volume *= 2.0f;
	mc->platform()->playSound(name, volume, pitch);
}
#elif defined(__APPLE__)
void SoundEngine::play(const std::string& name, float x, float y, float z, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp(volume, 0.0f, 1.0f);

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		soundSystem.playAt(sound, x-_x, y-_y, z-_z, volume, pitch);
	}
}
void SoundEngine::playUI(const std::string& name, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp(volume, 0.0f, 1.0f);
	if (/*!loaded || */options->sound == 0 || volume <= 0) return;

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		soundSystem.playAt(sound, 0, 0, 0, volume, pitch);
	}
}
#elif defined(RPI)
void SoundEngine::play(const std::string& name, float x, float y, float z, float volume, float pitch) {}
void SoundEngine::playUI(const std::string& name, float volume, float pitch) {}
#else
void SoundEngine::play(const std::string& name, float x, float y, float z, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp( volume * _getVolumeMult(x, y, z), 0.0f, 1.0f);
	if (/*!loaded || */options->sound == 0 || volume <= 0) return;

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		float dist = SOUND_DISTANCE;
		if (volume > 1) dist *= volume;
		soundSystem.playAt(sound, x - _x, y - _y, z - _z, volume, pitch);
	}
}
void SoundEngine::playUI(const std::string& name, float volume, float pitch) {
	if ((volume *= options->sound) <= 0) return;

	volume = Mth::clamp(volume, 0.0f, 1.0f);
	if (/*!loaded || */options->sound == 0 || volume <= 0) return;

	SoundDesc sound;
	if (sounds.get(name, sound)) {
		soundSystem.playAt(sound, 0, 0, 0, volume, pitch);
	}
}
#endif

// ── Background music ──────────────────────────────────────────────────────────
#if !defined(ANDROID) && !defined(RPI)

static const char* MUSIC_TRACKS[] = {
    "data/music/calm1.wav",
    "data/music/calm2.wav",
    "data/music/calm3.wav",
    nullptr
};

bool SoundEngine::_loadWav(const char* path, MusicTrack& out)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    char id[4];
    uint32_t sz;

    // RIFF header
    if (fread(id, 1, 4, f) != 4 || memcmp(id, "RIFF", 4) != 0) { fclose(f); return false; }
    fread(&sz, 4, 1, f);
    if (fread(id, 1, 4, f) != 4 || memcmp(id, "WAVE", 4) != 0) { fclose(f); return false; }

    int channels = 0, sampleRate = 0, byteWidth = 0;
    char* pcm = nullptr;
    int   pcmSize = 0;

    while (fread(id, 1, 4, f) == 4) {
        uint32_t chunkSz = 0;
        if (fread(&chunkSz, 4, 1, f) != 1) break;

        if (memcmp(id, "fmt ", 4) == 0) {
            uint16_t fmt, ch, blk, bits; uint32_t sr, br;
            fread(&fmt, 2, 1, f); fread(&ch, 2, 1, f);
            fread(&sr,  4, 1, f); fread(&br, 4, 1, f);
            fread(&blk, 2, 1, f); fread(&bits, 2, 1, f);
            if (chunkSz > 16) fseek(f, chunkSz - 16, SEEK_CUR);
            channels = ch; sampleRate = sr; byteWidth = bits / 8;
        } else if (memcmp(id, "data", 4) == 0) {
            pcm = new char[chunkSz];
            pcmSize = (int)fread(pcm, 1, chunkSz, f);
            break;
        } else {
            fseek(f, chunkSz, SEEK_CUR);
        }
    }
    fclose(f);

    if (!pcm || channels == 0 || sampleRate == 0 || byteWidth == 0) {
        delete[] pcm; return false;
    }
    out.data = pcm; out.size = pcmSize;
    out.channels = channels; out.sampleRate = sampleRate; out.byteWidth = byteWidth;
    return true;
}

void SoundEngine::_initMusic()
{
    for (int i = 0; MUSIC_TRACKS[i]; i++) {
        MusicTrack t{};
        if (_loadWav(MUSIC_TRACKS[i], t))
            _musicTracks.push_back(t);
    }
}

void SoundEngine::_tickMusic(bool inWorld)
{
    // Respect the music volume setting
    float vol = options ? options->music : 0.0f;
    if (vol <= 0.0f) {
        if (soundSystem.isMusicPlaying()) soundSystem.stopMusic();
        return;
    }

    if (!inWorld) {
        if (_musicWasInWorld && soundSystem.isMusicPlaying())
            soundSystem.stopMusic();
        _musicWasInWorld = false;
        _musicCooldownEndMs = 0;
        return;
    }

    long long now = (long long)getTimeMs();

    if (!_musicWasInWorld) {
        // Just entered a world — schedule first track after 5 minutes
        _musicWasInWorld = true;
        _musicCooldownEndMs = now + (90LL * 1000);
        return;
    }

    if (soundSystem.isMusicPlaying()) return;

    if (_musicCooldownEndMs == -1) {
        // Track just finished — pick a 2–5 minute cooldown before the next one
        long long wait = (2LL * 60 * 1000) + (now % (3LL * 60 * 1000));
        _musicCooldownEndMs = now + wait;
        return;
    }

    if (now < _musicCooldownEndMs) return; // still waiting

    _playNextMusicTrack();
    _musicCooldownEndMs = -1; // signals "detect track end → start cooldown"
}

void SoundEngine::_playNextMusicTrack()
{
    if (_musicTracks.empty()) return;
    float vol = options ? options->music : 1.0f;
    const MusicTrack& t = _musicTracks[_musicTrackIdx % (int)_musicTracks.size()];
    _musicTrackIdx++;
    soundSystem.playMusic(t.data, t.size, t.channels, t.sampleRate, t.byteWidth, vol);
}

#endif
