/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef LAUNCHINTOSPACECOMMAND_H_
#define LAUNCHINTOSPACECOMMAND_H_

#include "server/zone/objects/intangible/ShipControlDevice.h"
#include "server/zone/objects/ship/ShipObject.h"

class LaunchIntoSpaceCommand : public QueueCommand {
public:

	LaunchIntoSpaceCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const 
	{
		Logger::console.info("Hello Space!");
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;
		
		ZoneServer* zoneServer = server->getZoneServer();

        Reference<SceneObject*> terminal = server->getZoneServer()->getObject(target);

        if (terminal == nullptr || terminal->getGameObjectType() != SceneObjectType::SPACETERMINAL)
            return INVALIDSTATE;

		uint64 shipID;

		try 
		{
           	StringTokenizer tokenizer(arguments.toString());
			shipID = tokenizer.getLongToken();
        } 
		catch(Exception& e) 
		{
            return INVALIDPARAMETERS;
        }

		ManagedReference<ShipControlDevice*> pcd = zoneServer->getObject(shipID).castTo<ShipControlDevice*>();

		if (pcd == nullptr)
		{
			creature->error("Error retrieving ship");
			return GENERALERROR;
		}
		creature->switchZone("space_tatooine", 0, 0, 0);
		pcd->generateObject(creature);
		
		return SUCCESS;
	}

};

#endif //LAUNCHINTOSPACECOMMAND_H_

