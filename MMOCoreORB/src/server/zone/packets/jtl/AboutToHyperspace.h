/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef ABOUTTOHYPERSPACE_H_
#define ABOUTTOHYPERSPACE_H_

#include "server/zone/packets/object/ObjectControllerMessage.h"

class AboutToHyperspace : public ObjectControllerMessage {
public:
	/**
	 */
		AboutToHyperspace(SceneObject* player, uint32 shipID, String destinationZone, Vector3 destinationPosition)
			: ObjectControllerMessage(player->getObjectID(), 0x1B, 0x3FE, false) {

		insertShort(0x09);
		insertInt(shipID);
		insertAscii(destinationZone);
		insertFloat(destinationPosition.getX());
		insertFloat(destinationPosition.getY());
		insertFloat(destinationPosition.getZ());
	}
};

#endif