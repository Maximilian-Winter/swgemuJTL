/*
 * JtlShipListResponse.h
 *
 *  Created on: Apr 25, 2011
 *      Author: crush
 */

#ifndef JTLSHIPLISTRESPONSE_H_
#define JTLSHIPLISTRESPONSE_H_

#include "ObjectControllerMessage.h"
#include "server/zone/objects/creature/CreatureObject.h"
#include "server/zone/objects/intangible/ShipControlDevice.h"
#include "server/zone/objects/ship/ShipObject.h"

//TODO: This is very unsafe still...
class JtlShipListResponse : public ObjectControllerMessage {
public:
	JtlShipListResponse(CreatureObject* creo, SceneObject* terminal)
		: ObjectControllerMessage(creo->getObjectID(), 0x0B, 0x41D) 
	{
		
		SceneObject* datapad = creo->getSlottedObject("datapad");
		Vector<Reference<ShipObject*>> ships;
		for (int i = 0; i < datapad->getContainerObjectsSize(); i++) 
		{
			Reference<SceneObject*> obj =  datapad->getContainerObject(i).castTo<SceneObject*>();

			if (obj != nullptr && obj->isShipControlDevice() )
			{
				Reference<ShipControlDevice*> shipController = obj.castTo<ShipControlDevice*>();
				ManagedReference<TangibleObject*> controlledObject = shipController->getControlledObject();
				Reference<ShipObject*>ship = controlledObject.castTo<ShipObject*>();
				ships.add(ship);
			}
		}

		insertInt(ships.size()+1); // size
		insertLong(terminal->getObjectID());
		insertAscii("tatooine");
		for (auto &ship : ships) 
		{
			insertLong(ship->getObjectID());
			insertAscii("tatooine");
		}

		/*insertInt(2); // size
		insertLong(terminal->getObjectID());
		insertAscii("tatooine");

		SceneObject* datapad = creo->getSlottedObject("datapad");
		VectorMap<uint64, ManagedReference<SceneObject*> >* datapadObjects = datapad->getContainerObjects();

		for (int i = 0; i < datapadObjects->size(); ++i) {
			ManagedReference<SceneObject*> datapadObject = datapadObjects->get(i);

			if (datapadObject->getGameObjectType() == SceneObjectType::SHIPCONTROLDEVICE) {
				ManagedReference<ShipControlDevice*> shipControlDevice = cast<ShipControlDevice*>( datapadObject.get());

				if (shipControlDevice->getControlledObject() != nullptr) {
					ManagedReference<ShipObject*> ship = cast<ShipObject*>( shipControlDevice->getControlledObject());

					insertLong(ship->getObjectID());
					insertAscii("tatooine"); //TODO: Fix to retrieve ship->getParkedLocation();
				}
			}
		}*/
	}
};

#endif /* JTLSHIPLISTRESPONSE_H_ */
