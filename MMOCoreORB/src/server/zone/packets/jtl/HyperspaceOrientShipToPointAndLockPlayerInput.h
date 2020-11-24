/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef HYPERSPACEORIENTSHIPTOPOINTANDLOCKPLAYERINPUTMESSAGE_H_
#define HYPERSPACEORIENTSHIPTOPOINTANDLOCKPLAYERINPUTMESSAGE_H_

#include "server/zone/packets/object/ObjectControllerMessage.h"

class HyperspaceOrientShipToPointAndLockPlayerInput : public ObjectControllerMessage {
public:
	/**
	 */
	HyperspaceOrientShipToPointAndLockPlayerInput(SceneObject* player, uint32 shipID, String destinationZone, Vector3 destinationPosition)
			: ObjectControllerMessage(player->getObjectID(), 0x1B, 0x42D, false) {

		insertShort(0x09);
		insertInt(shipID);
		insertAscii(destinationZone);
		insertFloat(destinationPosition.getX());
		insertFloat(destinationPosition.getY());
		insertFloat(destinationPosition.getZ());
	}
};

#endif