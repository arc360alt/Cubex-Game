#ifndef NET_MINECRAFT_NETWORK_PACKET__SetPlayerSkinPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetPlayerSkinPacket_H__

#include "../Packet.h"
#include <vector>
#include <cstdint>

class SetPlayerSkinPacket : public Packet
{
public:
	int entityId;
	uint16_t skinW;
	uint16_t skinH;
	std::vector<uint8_t> skinPixels;

	SetPlayerSkinPacket()
	:	entityId(0), skinW(0), skinH(0)
	{}

	SetPlayerSkinPacket(int entityId_, uint16_t w, uint16_t h, const std::vector<uint8_t>& pixels)
	:	entityId(entityId_), skinW(w), skinH(h), skinPixels(pixels)
	{}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETPLAYERSKIN));
		bitStream->Write(entityId);
		bitStream->Write(skinW);
		bitStream->Write(skinH);
		uint32_t sz = (uint32_t)skinPixels.size();
		bitStream->Write(sz);
		if (sz > 0)
			bitStream->Write((const char*)skinPixels.data(), sz);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(skinW);
		bitStream->Read(skinH);
		uint32_t sz = 0;
		bitStream->Read(sz);
		const uint32_t MAX_SKIN = 64u * 64u * 4u;
		if (sz > 0 && sz <= MAX_SKIN && bitStream->GetNumberOfUnreadBits() >= sz * 8u) {
			skinPixels.resize(sz);
			bitStream->Read((char*)skinPixels.data(), sz);
		}
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SetPlayerSkinPacket_H__*/
