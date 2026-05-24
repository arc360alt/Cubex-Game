#ifndef NET_MINECRAFT_NETWORK_PACKET__SetFoodDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetFoodDataPacket_H__

#include "../Packet.h"

class SetFoodDataPacket: public Packet {
public:
	SetFoodDataPacket()
	: hunger(20),
	  saturation(5.0f)
	{
	}

	SetFoodDataPacket(int hunger, float saturation)
	: hunger(hunger),
	  saturation(saturation)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETFOODDATA));
		bitStream->Write((signed char)hunger);
		bitStream->Write(saturation);
	}

	void read(RakNet::BitStream* bitStream)
	{
		signed char tmpHunger;
		bitStream->Read(tmpHunger);
		hunger = tmpHunger;
		bitStream->Read(saturation);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SetFoodDataPacket*)this);
	}

	int hunger;
	float saturation;
};

#endif