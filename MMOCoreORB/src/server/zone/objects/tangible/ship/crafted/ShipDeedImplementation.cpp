/*
 */

#include "server/zone/objects/tangible/ship/crafted/ShipDeed.h"
#include"server/zone/ZoneServer.h"
#include "server/zone/packets/object/ObjectMenuResponse.h"
#include "templates/tangible/ShipDeedTemplate.h"
#include "server/zone/objects/intangible/ShipControlDevice.h"
#include "server/zone/objects/ship/ShipObject.h"
#include "server/zone/objects/player/PlayerObject.h"
#include "server/zone/Zone.h"
#include "server/zone/managers/player/PlayerManager.h"

void ShipDeedImplementation::loadTemplateData(SharedObjectTemplate* templateData) {
	DeedImplementation::loadTemplateData(templateData);

	ShipDeedTemplate* deedData = dynamic_cast<ShipDeedTemplate*>(templateData);

	if (deedData == nullptr)
		return;

	controlDeviceObjectTemplate = deedData->getControlDeviceObjectTemplate();
}

void ShipDeedImplementation::fillAttributeList(AttributeListMessage* alm, CreatureObject* object) {
	DeedImplementation::fillAttributeList(alm, object);

	alm->insertAttribute("hit_points", hitPoints);
}

void ShipDeedImplementation::initializeTransientMembers() {
	DeedImplementation::initializeTransientMembers();

	setLoggingName("ShipDeed");
}

void ShipDeedImplementation::updateCraftingValues(CraftingValues* values, bool firstUpdate) {
	/*
	 * Values available:	Range:
	 *
	 * hitpoints			varies, integrity of vehicle
	 */

	hitPoints = (int) values->getCurrentValue("hit_points");
}

void ShipDeedImplementation::fillObjectMenuResponse(ObjectMenuResponse* menuResponse, CreatureObject* player) {
	DeedImplementation::fillObjectMenuResponse(menuResponse, player);

	if (isASubChildOf(player))
		menuResponse->addRadialMenuItem(20, 3, "@pet/pet_menu:menu_generate");
}

int ShipDeedImplementation::handleObjectMenuSelect(CreatureObject* player, byte selectedID) {
	if (selectedID == 20) {
		if (generated || !isASubChildOf(player))
			return 1;

		/*if (player->isInCombat() || player->getParentRecursively(SceneObjectType::BUILDING) != nullptr) {
			player->sendSystemMessage("@pet/pet_menu:cant_call_vehicle"); //You can only unpack vehicles while Outside and not in Combat.
			return 1;
		}*/

		ManagedReference<SceneObject*> datapad = player->getSlottedObject("datapad");

		if (datapad == nullptr) {
			player->sendSystemMessage("Datapad doesn't exist when trying to create vehicle");
			return 1;
		}

		// Check if this will exceed maximum number of vehicles allowed
		ManagedReference<PlayerManager*> playerManager = player->getZoneServer()->getPlayerManager();

		int shipsInDatapad = 0;
		int maxStoredShips = playerManager->getBaseStoredShips();

		for (int i = 0; i < datapad->getContainerObjectsSize(); i++) {
			Reference<SceneObject*> obj =  datapad->getContainerObject(i).castTo<SceneObject*>();

			if (obj != nullptr && obj->isShipControlDevice() )
				shipsInDatapad++;

		}

		if (shipsInDatapad >= maxStoredShips) {
			player->sendSystemMessage("@pet/pet_menu:has_max_vehicle"); // You already have the maximum number of vehicles that you can own.
			return 1;
		}

		Reference<ShipControlDevice*> shipControlDevice = (server->getZoneServer()->createObject(controlDeviceObjectTemplate.hashCode(), 1)).castTo<ShipControlDevice*>();

		if (shipControlDevice == nullptr) {
			player->sendSystemMessage("wrong ship control device object template " + controlDeviceObjectTemplate);
			return 1;
		}

		Locker locker(shipControlDevice);

		Reference<ShipObject*> ship = (server->getZoneServer()->createObject(generatedObjectTemplate.hashCode(), 1)).castTo<ShipObject*>();

		if (ship == nullptr) {
			shipControlDevice->destroyObjectFromDatabase(true);
			player->sendSystemMessage("wrong ship object template " + generatedObjectTemplate);
			return 1;
		}
		Locker vlocker(ship, player);
		ship->createChildObjects();
		ship->setMaxCondition(hitPoints);
		ship->setConditionDamage(0);
		shipControlDevice->setControlledObject(ship);
		shipControlDevice->transferObject(ship, 4);
		if (datapad->transferObject(shipControlDevice, -1)) 
		{
			datapad->broadcastObject(shipControlDevice, true);

			destroyObjectFromWorld(true);
			destroyObjectFromDatabase(true);

			return 0;
		} else {
			shipControlDevice->destroyObjectFromDatabase(true);
			return 1;
		}
	}

	return DeedImplementation::handleObjectMenuSelect(player, selectedID);
}

