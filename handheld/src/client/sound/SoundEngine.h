#ifndef NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__
#define NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__

//package net.minecraft.client.sound;

#if defined(ANDROID) && !defined(PRE_ANDROID23)
	#include "../../platform/audio/SoundSystemSL.h"
#elif defined(RPI)
	#include "../../platform/audio/SoundSystem.h"
#else
    #include "../../platform/audio/SoundSystemAL.h"
#endif
#include "SoundRepository.h"
#include "../../util/Random.h"
#include <vector>
#include <string>

class Minecraft;
class Mob;
class Options;

class SoundEngine
{
    static const int SOUND_DISTANCE = 16;

	#if defined(ANDROID) && !defined(PRE_ANDROID23)
		SoundSystemSL soundSystem;
	#elif defined(RPI)
	    SoundSystem soundSystem;
	#else
        SoundSystemAL soundSystem;
	#endif

	Options* options;
	int idCounter;
	//static bool loaded;
	Random random;
	//int noMusicDelay = random.nextInt(20 * 60 * 10);
	float _x;
	float _y;
	float _z;
    float _yRot;
	float _invMaxDistance;

public:
	SoundEngine(float maxDistance);

	~SoundEngine();

    void init(Minecraft* mc, Options* options);
	void destroy();

	void enable(bool status);

	void updateOptions();
    void update(Mob* player, float a);

	void play(const std::string& name, float x, float y, float z, float volume, float pitch);
	void playUI(const std::string& name, float volume, float pitch);

	float _getVolumeMult(float x, float y, float z);
private:
	void loadLibrary() {}
    SoundDesc _pp(const std::string& fn);

	SoundRepository sounds;
	Minecraft* mc;

#if !defined(ANDROID) && !defined(RPI)
    // Background music
    struct MusicTrack {
        char* data;
        int size, channels, sampleRate, byteWidth;
    };
    std::vector<MusicTrack> _musicTracks;
    int  _musicTrackIdx;
    bool _musicWasInWorld;
    long long _musicCooldownEndMs; // -1 = track just ended, awaiting next cooldown

    void _initMusic();
    void _tickMusic(bool inWorld);
    void _playNextMusicTrack();
    static bool _loadWav(const char* path, MusicTrack& out);
#endif
};

#endif /*NET_MINECRAFT_CLIENT_SOUND__SoundEngine_H__*/
