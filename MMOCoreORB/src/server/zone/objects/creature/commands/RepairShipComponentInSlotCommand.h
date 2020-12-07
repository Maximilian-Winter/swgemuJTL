/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef REPAIRSHIPCOMPONENTINSLOTCOMMAND_H_
#define REPAIRSHIPCOMPONENTINSLOTCOMMAND_H_
#include "server/zone/objects/ship/ShipObject.h"

class RepairShipComponentInSlotCommand : public QueueCommand {
public:

	RepairShipComponentInSlotCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const {

		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;
		
		StringTokenizer tokenizer(arguments.toString());
		long shipId = tokenizer.getLongToken();
        int slot = tokenizer.getIntToken();

        ManagedReference<SceneObject*> targetSceno = server->getZoneServer()->getObject(target);

        ManagedReference<SceneObject*> shipSceno = server->getZoneServer()->getObject(shipId);
        ManagedReference<ShipObject*> ship = shipSceno.castTo<ShipObject*>();

        Locker locker(ship);
		ship->setCurrentChassisHealth(900.0f);
		ship->setCapacitorEnergy(1000.0f);
		return SUCCESS;
	}

};

#endif //REPAIRSHIPCOMPONENTINSLOTCOMMAND_H_
