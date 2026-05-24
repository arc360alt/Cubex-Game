#include "NetEventCallback.h"
#include "../world/level/Level.h"
#include "../world/entity/player/Player.h"
#include "packet/RespawnPacket.h"

//
// Common packet handling implementation for Client and Server
//
/*
void NetEventCallback::handle( const RakNet::RakNetGUID& source, AnimatePacket* packet )
{
	Entity* entity = level->getEntity(packet->entityId);
	if (entity && entity->isPlayer()) {
		Player* player = (Player*) entity;

		switch (packet->action) {
		case AnimatePacket::Swing:
			player->swing();
			break;
		default:
			LOGW("Unknown Animate action: %d\n", packet->action);
			break;
		}
	}
}
*/


Player* NetEventCallback::findPlayer(Level* level, int entityId) {
	Entity* e = level->getEntity(entityId);
	if (e) {
		if (e->isPlayer()) return (Player*) e;
		LOGE("Entity: %p is supposed to be a player but is not (type %d)!\n", e, e->getEntityTypeId());
	}
	return NULL;
}

Player* NetEventCallback::findPlayer(Level* level, const RakNet::RakNetGUID* source) {
	for (unsigned int i = 0; i < level->players.size(); ++i)
		if (level->players[i]->owner == *source) return level->players[i];
	return NULL;
}

Player* NetEventCallback::findPlayer( Level* level, int entityId, const RakNet::RakNetGUID* source )
{
	if (entityId != -1)
		if (Player* p = findPlayer(level, entityId)) return p;
	if (source != NULL)
		if (Player* p = findPlayer(level, source)) return p;
	return NULL;
}

void NetEventCallback::handle( Level* level, const RakNet::RakNetGUID& source, RespawnPacket* packet )
{
	// Fall back to GUID lookup in case entity IDs differ between client and server
	if (Player* p = findPlayer(level, packet->entityId, &source)) {
		p->moveTo(packet->x, packet->y, packet->z, p->yRot, p->xRot);
		p->reset();
		p->health = Player::MAX_HEALTH;    // reset() only gives mobs 10 HP; players need 20
		p->invulnerableTime = 0;
		p->foodData.setHunger(20);
		p->foodData.setSaturation(20.0f);
		p->resetPos(true);
	}
}
