#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

//package net.minecraft.world.level.levelgen.synth;

#include "Synth.h"
class Random;

class ImprovedNoise: public Synth
{
public:
    ImprovedNoise();

    ImprovedNoise(Random* random);
	
	void init(Random* random);

    float noise(float _x, float _y, float _z);

    const float lerp(float t, float a, float b);

    const float grad2(int hash, float x, float z);
    const float grad(int hash, float x, float y, float z);

    float getValue(float x, float y);
    float getValue(float x, float y, float z);

    void add(float* buffer, float _x, float _y, float _z, int xSize, int ySize, int zSize, float xs, float ys, float zs, float pow);

    int hashCode();

	float scale;
	float xo, yo, zo;
private:
	int p[512];

	// (int)x in Java saturates to INT32_MAX/INT32_MIN (and maps NaN to 0)
	// instead of invoking undefined behavior like C++'s (int) cast does once
	// x no longer fits in 32 bits. At extreme world coordinates (~12.55M+
	// blocks, matching vanilla's "Far Lands" distance) this coordinate does
	// overflow int32 - saturating here keeps the result finite so the noise
	// degenerates into vanilla's classic jagged Far Lands terrain instead of
	// cascading into Infinity/NaN and silently generating nothing at all.
	static int floatToIntSaturating(float x);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__*/
