/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef INSTALLSHIPCOMPONENTCOMMAND_H_
#define INSTALLSHIPCOMPONENTCOMMAND_H_3
#include "server/zone/objects/ship/ShipObject.h"
#include "server/zone/objects/ship/components/ShipComponent.h"

class InstallShipComponentCommand : public QueueCommand {
public:

	InstallShipComponentCommand(const String& name, ZoneProcessServer* server)
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
        String componentIdString = tokenizer.getStringToken();

		for(int i = 0; i < componentIdString.length(); i++)
		{
			if(!std::isdigit(componentIdString[i]))
			{
				return INVALIDPARAMETERS;
			}
		}

		long componentId = Long::valueOf(componentIdString);

        ManagedReference<SceneObject*> targetSceno = server->getZoneServer()->getObject(target);

        ManagedReference<SceneObject*> component = server->getZoneServer()->getObject(componentId);
        ManagedReference<SceneObject*> shipSceno = server->getZoneServer()->getObject(shipId);
        ManagedReference<ShipObject*> ship = shipSceno.castTo<ShipObject*>();
		ManagedReference<ShipComponent*> shipComp = component.castTo<ShipComponent*>();

		//info("CompDataName" + shipComp->getComponentDataName(), true);
        Locker locker(ship);
        info("Attempting to install component(" + String::valueOf((int64)componentId) + ") at slot: " + String::valueOf(slot), true);
        ship->install(creature, component, slot);
		return SUCCESS;
	}

};

#endif //INSTALLSHIPCOMPONENTCOMMAND_H_
