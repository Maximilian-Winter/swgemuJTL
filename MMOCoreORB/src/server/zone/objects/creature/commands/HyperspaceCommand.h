/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#ifndef HYPERSPACECOMMAND_H_
#define HYPERSPACECOMMAND_H_

#include "server/zone/objects/player/events/HyperspaceTask.h"
#include "server/zone/managers/space/SpaceManager.h"

class HyperspaceCommand : public QueueCommand {
public:

	HyperspaceCommand(const String& name, ZoneProcessServer* server)
		: QueueCommand(name, server) {

	}

	int doQueueCommand(CreatureObject* creature, const uint64& target, const UnicodeString& arguments) const 
	{
		Logger::console.info("Hello Hyperspace!");
		if (!checkStateMask(creature))
			return INVALIDSTATE;

		if (!checkInvalidLocomotions(creature))
			return INVALIDLOCOMOTION;

		ManagedReference<SceneObject*> ship = creature->getParent();
		ManagedReference<SpaceManager*> spaceManager = (SpaceManager*)creature->getZone()->getPlanetManager();


        Locker lock(creature, ship);

        String targetPoint;

        try 
		{
            UnicodeTokenizer tokenizer(arguments);
            tokenizer.getStringToken(targetPoint);
        } 
		catch(Exception& e) 
		{
            return INVALIDPARAMETERS;
        }
		HyperspaceTravelPoint* travelPoint = spaceManager->getHyperspaceTravelPoint(targetPoint);
		if(travelPoint != nullptr)
		{
			Reference<HyperspaceTask*> hyperspaceTask = new HyperspaceTask(creature);

			hyperspaceTask->setTargetZone(travelPoint->getPointZone());
			hyperspaceTask->setTargetPosition(travelPoint->getHyperspacePosition());

			creature->sendSystemMessage("Hyperspace route calculation begun. Status: 0%");

			creature->addPendingTask("hyperspace", hyperspaceTask, 10000);
			

			return SUCCESS;
		}
		else
		{
			return INVALIDPARAMETERS;
		}
				
		
	}

};

#endif 
