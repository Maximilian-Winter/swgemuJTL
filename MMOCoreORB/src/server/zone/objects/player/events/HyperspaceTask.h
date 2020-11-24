#define HYPERSPACETASK_H_

#include "engine/engine.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "templates/params/creature/CreatureAttribute.h"
#include "server/zone/packets/jtl/HyperspaceOrientShipToPointAndLockPlayerInput.h"
#include "server/zone/packets/jtl/AboutToHyperspace.h"

class HyperspaceTask: public Task {
	ManagedReference<CreatureObject*> player;
	String targetZone;
	Vector3 targetPosition;
public:
	HyperspaceTask(CreatureObject* pl) {
		player = pl;
	}

	void setTargetZone(const String& tz) {
		targetZone = tz;
	}

	String getTargetZone() {
		return targetZone;
	}

	void setTargetPosition(const Vector3& tp) {
		targetPosition = tp;
	}

	Vector3 getTargetPosition() {
		return targetPosition;
	}


	int timeLeft = 45;


	void run() {
		Locker playerLocker(player);

		try {

			Reference<HyperspaceTask*> hyperspaceTask = player->getPendingTask("hyperspace").castTo<HyperspaceTask*>();

			ManagedReference<SceneObject*> ship = player->getParent();

			Locker lock(player, ship);
			if (timeLeft == 45)
			{
				player->sendSystemMessage("Hyperspace route calculating. Status: 25%");
				timeLeft -= 10;
				hyperspaceTask->reschedule(10000);
			}
			else if (timeLeft == 35)
			{
				player->sendSystemMessage("Hyperspace route calculating. Status: 50%");
				timeLeft -= 10;
				hyperspaceTask->reschedule(10000);
			}
			else if (timeLeft == 25)
			{
				player->sendSystemMessage("Hyperspace route calculating. Status: 75%");
				timeLeft -= 10;
				hyperspaceTask->reschedule(10000);
			}
			else if (timeLeft == 15)
			{
				player->sendSystemMessage("Hyperspace route calculating. Status: 100%");
				timeLeft -= 1;
				hyperspaceTask->reschedule(1000);
			}	
			else if (timeLeft == 14)			
			{
				HyperspaceOrientShipToPointAndLockPlayerInput* orient = new HyperspaceOrientShipToPointAndLockPlayerInput(player, ship->getObjectID(), targetZone, targetPosition);
				player->sendSystemMessage("Hyperspace route calculations complete. Jumping to Hyperspace in 5.");
				player->sendMessage(orient);
				timeLeft -= 1;
				hyperspaceTask->reschedule(1000);
			} 
			else if (timeLeft == 13)
			{
				player->sendSystemMessage("4");
				timeLeft -= 1;	
				hyperspaceTask->reschedule(1000);
			}
			else if (timeLeft == 12)
			{
				player->sendSystemMessage("3");
				timeLeft -= 1;
				hyperspaceTask->reschedule(1000);
			}
			else if (timeLeft == 11)
			{
				player->sendSystemMessage("2");
				timeLeft -= 1;
				hyperspaceTask->reschedule(1000);
			}
			else if (timeLeft == 10)
			{	
				player->sendSystemMessage("1");			
				AboutToHyperspace* ATH = new AboutToHyperspace(player, ship->getObjectID(), targetZone, targetPosition);
				player->sendMessage(ATH);
				timeLeft -= 10;
				hyperspaceTask->reschedule(12000);
			}
			else if (timeLeft == 0)
			{
				Locker lock(ship, player);
				player->removePendingTask("hyperspace");
				player->switchZone(targetZone, targetPosition.getX(), targetPosition.getZ(), targetPosition.getY(), 0, 0);
				ship->switchZone(targetZone, targetPosition.getX(), targetPosition.getZ(), targetPosition.getY(), 0, 0);
				ship->transferObject(player, 5, true);
			}
        }
        catch(Exception& e) 
		{
            Logger::console.info("ERROR In HyperspaceTask!");
        }
    }
};