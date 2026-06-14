#ifndef NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__

#include "../Packet.h"

class StartGamePacket : public Packet
{
public:
	enum {
		WORLD_TYPE_MASK = 1 << 30,
		LEVEL_GENERATOR_MASK = WORLD_TYPE_MASK - 1,
	};

	long levelSeed;
	int levelGeneratorVersion;
	int gameType;

	int entityId;
	float x, y, z;
	float yRot, xRot;

	StartGamePacket()
	: yRot(0), xRot(0)
	{
	}

	StartGamePacket(long seed, int levelGeneratorVersion, int gameType, int worldType, int entityId, float x, float y, float z, float yRot = 0, float xRot = 0)
	:	levelSeed(seed),
		levelGeneratorVersion((levelGeneratorVersion & LEVEL_GENERATOR_MASK) | (worldType == 1 ? WORLD_TYPE_MASK : 0)),
		gameType(gameType),
		entityId(entityId),
		x(x),
		y(y),
		z(z),
		yRot(yRot),
		xRot(xRot)
	{
	}

	int getWorldType() const
	{
		return (levelGeneratorVersion & WORLD_TYPE_MASK) ? 1 : 0;
	}

	int getLevelGeneratorVersion() const
	{
		return levelGeneratorVersion & LEVEL_GENERATOR_MASK;
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_STARTGAME));

		bitStream->Write(levelSeed);
		bitStream->Write(levelGeneratorVersion);
		bitStream->Write(gameType);
		// Write worldType explicitly so the reader's backward-compat heuristic
		// fires correctly and doesn't misread entityId as worldType.
		int wt = getWorldType();
		bitStream->Write(wt);
		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(yRot);
		bitStream->Write(xRot);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(levelSeed);
		bitStream->Read(levelGeneratorVersion);
		bitStream->Read(gameType);

		// Compatibility:
		// - Old layout:               seed, gen, gameType, entityId, x, y, z
		// - Transitional broken one:  seed, gen, gameType, worldType, entityId, x, y, z
		// - Packed layout:            seed, packed(gen+worldType), gameType, entityId, x, y, z
		// If there are enough unread bits for an extra int before entityId,
		// treat that int as explicit worldType and fold it into packed form.
		if (bitStream->GetNumberOfUnreadBits() >= (sizeof(int) * 2 + sizeof(float) * 3) * 8)
		{
			int explicitWorldType = 0;
			bitStream->Read(explicitWorldType);
			if (explicitWorldType == 1)
				levelGeneratorVersion |= WORLD_TYPE_MASK;
			else
				levelGeneratorVersion &= LEVEL_GENERATOR_MASK;
		}

		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		// Optional: yRot/xRot (older servers won't send these)
		if (bitStream->GetNumberOfUnreadBits() >= (sizeof(float) * 2) * 8) {
			bitStream->Read(yRot);
			bitStream->Read(xRot);
		}
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (StartGamePacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__StartGamePacket_H__*/
