/*
 * CreateMissileMessage.h
 *
 *  Created on: Nov 20, 2008
 *      Author: swgemu
 */

#ifndef CREATEMISSILEMESSAGE_H_
#define CREATEMISSILEMESSAGE_H_

#include "engine/service/proto/BaseMessage.h"
#include "server/zone/packets/ship/PackedPosition.h"
#include "server/zone/managers/ship/ShipManager.h"
#include "server/zone/objects/ship/components/ShipWeaponComponent.h"

class CreateMissileMessage : public BaseMessage {
public:
	CreateMissileMessage() : BaseMessage() {
		insertShort(0x14);
		insertInt(0x721CF08B);  // CRC

		insertInt(0); //MissileID
		insertLong(0); //ShipID
		insertLong(0); //TargetID
		insertInt(0); //SourcePosition
		insertInt(0); 
		insertInt(0);
		insertInt(0); //TargetPosition
		insertInt(0);
		insertInt(0);
		insertInt(0); // ImpactTime
		insertInt(0); // MissleType
		insertInt(0); // WeaponID
		insertInt(0); // TargetComponent
   }

};


class CreateMissileMessageCallback : public MessageCallback {
	uint missileID;
	Long shipID;
	Long targetID;
	Vector3 sourcePosition;
	Vector3 targetPosition;
	uint impactTime;
	uint missleType;
	uint weaponID;
	uint targetComponent;

	ObjectControllerMessageCallback* objectControllerMain;
public:
	CreateMissileMessageCallback(ZoneClientSession* client, ZoneProcessServer* server) :
			MessageCallback(client, server) {

	}

	void parse(Message* message) {

		missileID = message->parseInt();
		shipID = message->parseLong();
		targetID = message->parseLong();

 		uint spX = message->parseInt();
		uint spY = message->parseInt();
		uint spZ = message->parseInt();
		sourcePosition = Vector3(spX, spY, spZ);

		uint tpX = message->parseInt();
		uint tpY = message->parseInt();
		uint tpZ = message->parseInt();
		targetPosition = Vector3(tpX, tpY, tpZ);

		impactTime = message->parseInt();
		missleType = message->parseInt();
		weaponID = message->parseInt();
		targetComponent = message->parseInt();
	}

	void run() {
		StringBuffer buffer;
		buffer << "CreateMissileMessage: " << shipID << endl << "weapon: " << weaponID << endl << "missileType:" << missleType << endl;
		buffer << "ComponentIndex:" << targetComponent << endl << "sourceposition: " << sourcePosition.toString() << endl << "targetpos: " << targetPosition.toString() << endl << "missileID: " << missileID << endl;
		static Logger logger;
		logger.info(buffer.toString(), true);
/*
		//
		ManagedReference<CreatureObject*> object = client->getPlayer();

		if (object == nullptr)
			return;

		Locker _locker(object);

		ManagedReference<ShipObject*> ship = dynamic_cast<ShipObject*>(object->getParent().get().get());

		if (ship == nullptr)
			return;

		String name = ship->getComponentNameMap()->get(ShipObject::WEAPON_COMPONENT_START+weaponIndex).toString();
		const ShipMissileData* data = ShipManager::instance()->getMissileData(name.hashCode());

		ShipWeaponComponent *weapon = cast<ShipWeaponComponent*>(ship->getComponentObject(ShipObject::WEAPON_COMPONENT_START+weaponIndex));

		if (weapon == nullptr) {
			ship->error("Attempted to create projectile with missing weapon : " + String::valueOf(weaponIndex));
			return;
		}

		float currentEnergy = ship->getCapacitorEnergy();
		float cost = weapon->getEnergyPerShot();
		if (cost > currentEnergy) {
			ship->error("Attempted to create projectile with insufficient energy");
			return;
		}

		Locker cross(ship, object);
		ship->setCapacitorEnergy(currentEnergy-cost, true);

		// TODO: Validate shot
		ShipManager::ShipMissile* projectile = new ShipManager::ShipMissile(ship, weaponIndex, projectileType, componentIndex, position, direction, data->getSpeed(), data->getRange(), System::getMiliTime());

		// compensate latency for outgoing message
		ShipManager::instance()->addMissile(projectile);
		CreateMissileMessage *message = new CreateMissileMessage(position, direction, componentIndex, projectileType, weaponIndex, ship->getUniqueID(), sequence);

		object->broadcastMessage(message, false);*/
	}

	const char* getTaskName() {
		return "CreateMissileMessage";
	}
};


#endif /* CREATEMISSILEMESSAGE_H_ */
