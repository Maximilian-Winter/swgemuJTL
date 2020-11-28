/*
				Copyright <SWGEmu>
		See file COPYING for copying conditions.*/

#include "server/zone/Zone.h"

#include "server/zone/ZoneProcessServer.h"
#include "server/zone/objects/scene/SceneObject.h"
#include "server/zone/managers/planet/PlanetManager.h"
#include "server/zone/managers/space/SpaceManager.h"
#include "server/zone/managers/creature/CreatureManager.h"
#include "server/zone/managers/components/ComponentManager.h"
#include "server/zone/packets/player/GetMapLocationsResponseMessage.h"

#include "server/zone/objects/cell/CellObject.h"
#include "server/zone/objects/region/Region.h"
#include "server/zone/objects/building/BuildingObject.h"
#include "server/zone/objects/tangible/terminal/Terminal.h"
#include "templates/SharedObjectTemplate.h"

#include "server/zone/managers/structure/StructureManager.h"
#include "terrain/ProceduralTerrainAppearance.h"
#include "server/zone/managers/collision/NavMeshManager.h"

ZoneImplementation::ZoneImplementation(ZoneProcessServer* serv, const String& name) {
	processor = serv;
	server = processor->getZoneServer();

	zoneName = name;
	zoneCRC = name.hashCode();

	regionTree = new server::zone::OctTree(-8192, -8192, -8192, 8192, 8192, 8192);
	octTree = new server::zone::OctTree(-8192, -8192, -8192, 8192, 8192, 8192);

	objectMap = new ObjectMap();

	mapLocations = new MapLocationTable();

	managersStarted = false;
	zoneCleared = false;

	//galacticTime = new Time();

	planetManager = nullptr;

	setLoggingName("Zone " + name);

	Core::getTaskManager()->initializeCustomQueue(zoneName, 1, true);
}

void ZoneImplementation::createContainerComponent() {
	containerComponent = ComponentManager::instance()->getComponent<ContainerComponent*>("ZoneContainerComponent");
}

void ZoneImplementation::initializePrivateData() {
	if (zoneName.contains("space_")) {
		planetManager = new SpaceManager(_this.getReferenceUnsafeStaticCast(), processor);
	} else {
		planetManager = new PlanetManager(_this.getReferenceUnsafeStaticCast(), processor);
	}

	creatureManager = new CreatureManager(_this.getReferenceUnsafeStaticCast());
	creatureManager->deploy("CreatureManager " + zoneName);
	creatureManager->setZoneProcessor(processor);
}

void ZoneImplementation::finalize() {
	//System::out << "deleting height map\n";
}

void ZoneImplementation::initializeTransientMembers() {
	ManagedObjectImplementation::initializeTransientMembers();

	mapLocations = new MapLocationTable();

	//heightMap->load("planets/" + planetName + "/" + planetName + ".hmap");
}

void ZoneImplementation::startManagers() {
	planetManager->initialize();

	creatureManager->initialize();

	StructureManager::instance()->loadPlayerStructures(getZoneName());

	ObjectDatabaseManager::instance()->commitLocalTransaction();

	planetManager->start();

	managersStarted = true;
}

void ZoneImplementation::stopManagers() {
	info("Shutting down.. ", true);

	if (creatureManager != nullptr) {
		creatureManager->stop();
		creatureManager = nullptr;
	}

	if (planetManager != nullptr) {
		//planetManager->finalize();
		planetManager = nullptr;
	}

	processor = nullptr;
	server = nullptr;
	mapLocations = nullptr;
	objectMap = nullptr;
	octTree = nullptr;
	regionTree = nullptr;
}

void ZoneImplementation::clearZone() {
	Locker zonelocker(_this.getReferenceUnsafeStaticCast());

	info("clearing zone", true);

	creatureManager->unloadSpawnAreas();

	HashTable<uint64, ManagedReference<SceneObject*> > tbl;
	tbl.copyFrom(*objectMap->getMap());

	zonelocker.release();

	auto iterator = tbl.iterator();

	while (iterator.hasNext()) {
		ManagedReference<SceneObject*> sceno = iterator.getNextValue();

		if (sceno != nullptr) {
			Locker locker(sceno);
			sceno->destroyObjectFromWorld(false);
		}
	}

	Locker zonelocker2(_this.getReferenceUnsafeStaticCast());

	zoneCleared = true;

	info("zone clear", true);
}

float ZoneImplementation::getHeight(float x, float y) {
	if (planetManager != nullptr) {
		TerrainManager* manager = planetManager->getTerrainManager();

		if (manager != nullptr)
			return manager->getHeight(x, y);
	}

	return 0;
}

float ZoneImplementation::getHeightNoCache(float x, float y) {
	if (planetManager != nullptr) {
		TerrainManager* manager = planetManager->getTerrainManager();

		if (manager != nullptr) {
			ProceduralTerrainAppearance *appearance = manager->getProceduralTerrainAppearance();
			if (appearance != nullptr)
				return appearance->getHeight(x, y);
		}
	}

	return 0;
}

void ZoneImplementation::insert(OctTreeEntry* entry) {
	Locker locker(_this.getReferenceUnsafeStaticCast());

	octTree->insert(entry);
}

void ZoneImplementation::remove(OctTreeEntry* entry) {
	Locker locker(_this.getReferenceUnsafeStaticCast());

	if (entry->isInOctTree())
		octTree->remove(entry);
}

void ZoneImplementation::update(OctTreeEntry* entry) {
	Locker locker(_this.getReferenceUnsafeStaticCast());

	octTree->update(entry);
}

void ZoneImplementation::inRange(OctTreeEntry* entry, float range) {
	octTree->safeInRange(entry, range);
}

int ZoneImplementation::getInRangeSolidObjects(float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >* objects, bool readLockZone) {
	objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	try {
		_this.getReferenceUnsafeStaticCast()->rlock(readlock);

		octTree->inRange(x, y, z, range, *objects);

		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	} catch (...) {
		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	}

	if (objects->size() > 0) {
		for (int i = objects->size() - 1; i >= 0; i--) {
			SceneObject* sceno = static_cast<SceneObject*>(objects->getUnsafe(i).get());

			if (sceno == nullptr || sceno->getParentID() != 0) {
				objects->remove(i);
				continue;
			}

			if (sceno->isCreatureObject() || sceno->isLairObject()) {
				objects->remove(i);
				continue;
			}

			if (sceno->getGameObjectType() == SceneObjectType::FURNITURE) {
				objects->remove(i);
				continue;
			}

			SharedObjectTemplate *shot = sceno->getObjectTemplate();

			if (shot == nullptr) {
				objects->remove(i);
				continue;
			}

			if (!shot->getCollisionMaterialFlags() || !shot->getCollisionMaterialBlockFlags() || !shot->isNavUpdatesEnabled()) {
				objects->remove(i);
				continue;
			}
		}
	}
	return objects->size();
}

int ZoneImplementation::getInRangeObjects(float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >* objects, bool readLockZone, bool includeBuildingObjects) {
	objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	try {
		_this.getReferenceUnsafeStaticCast()->rlock(readlock);

		int countOctTree = octTree->inRange(x, y, z, range, *objects);

		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	} catch (...) {
		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	}

	if (includeBuildingObjects) {
		Vector<ManagedReference<OctTreeEntry*> > buildingObjects;

		for (int i = 0; i < objects->size(); ++i) {
			SceneObject* sceneObject = static_cast<SceneObject*>(objects->getUnsafe(i).get());
			BuildingObject* building = sceneObject->asBuildingObject();

			if (building != nullptr) {
				for (int j = 1; j <= building->getMapCellSize(); ++j) {
					CellObject* cell = building->getCell(j);

					if (cell != nullptr && cell->isContainerLoaded()) {
						try {
							ReadLocker rlocker(cell->getContainerLock());

							for (int h = 0; h < cell->getContainerObjectsSize(); ++h) {
								Reference<SceneObject*> obj = cell->getContainerObject(h);

								if (obj != nullptr)
									buildingObjects.emplace(std::move(obj));
							}

						} catch (...) {
						}
					}
				}
			} else if (sceneObject->isVehicleObject() || sceneObject->isMount()) {
				Reference<SceneObject*> rider = sceneObject->getSlottedObject("rider");

				if (rider != nullptr)
					buildingObjects.emplace(std::move(rider));
			}
		}

		//_this.getReferenceUnsafeStaticCast()->runlock(readlock);

		for (int i = 0; i < buildingObjects.size(); ++i)
			objects->put(std::move(buildingObjects.getUnsafe(i)));
	}

	return objects->size();
}

int ZoneImplementation::getInRangeObjects(float x, float y, float z, float range, InRangeObjectsVector* objects, bool readLockZone, bool includeBuildingObjects) {
	objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	try {
		_this.getReferenceUnsafeStaticCast()->rlock(readlock);

		octTree->inRange(x, y, z, range, *objects);

		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	} catch (...) {
		_this.getReferenceUnsafeStaticCast()->runlock(readlock);
	}

	if (includeBuildingObjects) {
		Vector<OctTreeEntry*> buildingObjects;

		for (int i = 0; i < objects->size(); ++i) {
			SceneObject* sceneObject = static_cast<SceneObject*>(objects->getUnsafe(i));

			BuildingObject* building = sceneObject->asBuildingObject();

			if (building != nullptr) {
				for (int j = 1; j <= building->getMapCellSize(); ++j) {
					CellObject* cell = building->getCell(j);

					if (cell != nullptr && cell->isContainerLoaded()) {
						try {
							ReadLocker rlocker(cell->getContainerLock());

							for (int h = 0; h < cell->getContainerObjectsSize(); ++h) {
								Reference<SceneObject*> obj = cell->getContainerObject(h);

								if (obj != nullptr)
									buildingObjects.add(obj.get());
							}

						} catch (Exception& e) {
							warning("exception in Zone::GetInRangeObjects: " + e.getMessage());
						}
					}
				}
			} else if (sceneObject->isVehicleObject() || sceneObject->isMount()) {
				Reference<SceneObject*> rider = sceneObject->getSlottedObject("rider");

				if (rider != nullptr)
					buildingObjects.add(rider.get());
			}
		}

		for (int i = 0; i < buildingObjects.size(); ++i)
			objects->put(buildingObjects.getUnsafe(i));
	}

	return objects->size();
}

int ZoneImplementation::getInRangePlayers(float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >* players) {
	Reference<SortedVector<ManagedReference<OctTreeEntry*> >*> closeObjects = new SortedVector<ManagedReference<OctTreeEntry*> >();

	getInRangeObjects(x, y, z, range, closeObjects, true);

	for (int i = 0; i < closeObjects->size(); ++i) {
		SceneObject* object = cast<SceneObject*>(closeObjects->get(i).get());

		if (object == nullptr || !object->isPlayerCreature())
			continue;

		CreatureObject* player = object->asCreatureObject();

		if (player == nullptr || player->isInvisible())
			continue;

		players->emplace(object);
	}

	return players->size();
}

int ZoneImplementation::getInRangeActiveAreas(float x, float y, float z, SortedVector<ManagedReference<ActiveArea*> >* objects, bool readLockZone) {
	//objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	Zone* thisZone = _this.getReferenceUnsafeStaticCast();

	SortedVector<ManagedReference<OctTreeEntry*> > entryObjects;
	//SortedVector<ManagedReference<OctTreeEntry*> > entryObjects2;

	try {
		thisZone->rlock(readlock);

		regionTree->inRange(x, y, z, entryObjects);
		//regionTree->inRange(x, y, z, 1024, entryObjects2);

		thisZone->runlock(readlock);
	} catch (...) {
		thisZone->runlock(readlock);

		throw;
	}


	for (int i = 0; i < entryObjects.size(); ++i) {
		ActiveArea* obj = static_cast<ActiveArea*>(entryObjects.get(i).get());
		objects->put(obj);
	}

	/*for (int i = 0; i < entryObjects2.size(); ++i) {
		ActiveArea* obj = static_cast<ActiveArea*>(entryObjects2.get(i).get());

		if (obj->containsPoint(x, y, z))
			objects->put(obj);
	}*/

	return objects->size();
}

int ZoneImplementation::getInRangeNavMeshes(float x, float y, float z, SortedVector<ManagedReference<NavArea*> >* objects, bool readlock) {
	//objects->setNoDuplicateInsertPlan();

	Zone* thisZone = _this.getReferenceUnsafeStaticCast();

	SortedVector<OctTreeEntry*> entryObjects;
	//SortedVector<OctTreeEntry*> entryObjects2;

	ReadLocker rlocker(thisZone);

	regionTree->inRange(x, y, z, 256, entryObjects);

	//regionTree->inRange(x, y, z, 1024, entryObjects2);

	for (int i = 0; i < entryObjects.size(); ++i) {
		ActiveArea* area = static_cast<ActiveArea*>(entryObjects.getUnsafe(i));
		NavArea* obj = area->asNavArea();

		if (obj && obj->isNavMeshLoaded() && obj->isInRange(x, y, 256)) {
			objects->put(obj);
		}
	}

	/*for (int i = 0; i < entryObjects2.size(); ++i) {
		ActiveArea* area = static_cast<ActiveArea*>(entryObjects2.getUnsafe(i));
		NavArea* obj = area->asNavArea();

		if (obj && obj->isNavMeshLoaded() && obj->isInRange(x, y, 256))
			objects->put(obj);
	}*/

	return objects->size();
}

int ZoneImplementation::getInRangeActiveAreas(float x, float y, float z, ActiveAreasVector* objects, bool readLockZone) {
	//objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	Zone* thisZone = _this.getReferenceUnsafeStaticCast();

	SortedVector<OctTreeEntry*> entryObjects;
	//SortedVector<OctTreeEntry*> entryObjects2;

	try {
		thisZone->rlock(readlock);

		regionTree->inRange(x, y, z, entryObjects);
		//regionTree->inRange(x, y, z, 1024, entryObjects2);

		thisZone->runlock(readlock);
	} catch (...) {
		thisZone->runlock(readlock);

		throw;
	}


	for (int i = 0; i < entryObjects.size(); ++i) {
		ActiveArea* obj = static_cast<ActiveArea*>(entryObjects.getUnsafe(i));
		objects->put(obj);
	}
/*
	for (int i = 0; i < entryObjects2.size(); ++i) {
		ActiveArea* obj = static_cast<ActiveArea*>(entryObjects2.getUnsafe(i));

		if (obj->containsPoint(x, y, z))
			objects->put(obj);
	}*/

	return objects->size();
}

int ZoneImplementation::getInRangeActiveAreas(float x, float y, float z,float range, ActiveAreasVector* objects, bool readLockZone) {
	//Locker locker(_this.getReferenceUnsafeStaticCast());
	//objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	//_this.getReferenceUnsafeStaticCast()->rlock(readlock);

	Zone* thisZone = _this.getReferenceUnsafeStaticCast();

	try {
		SortedVector<OctTreeEntry*> entryObjects;

		thisZone->rlock(readlock);

		regionTree->inRange(x, y, z, range, entryObjects);

		thisZone->runlock(readlock);

		for (int i = 0; i < entryObjects.size(); ++i) {
			ActiveArea* obj = static_cast<ActiveArea*>(entryObjects.getUnsafe(i));
			objects->put(obj);
		}
	}catch (...) {
		//		_this.getReferenceUnsafeStaticCast()->runlock(readlock);

		throw;
	}

	//	_this.getReferenceUnsafeStaticCast()->runlock(readlock);

	return objects->size();
}

int ZoneImplementation::getInRangeActiveAreas(float x, float y, float z, float range, SortedVector<ManagedReference<ActiveArea*> >* objects, bool readLockZone) {
	//Locker locker(_this.getReferenceUnsafeStaticCast());
	//objects->setNoDuplicateInsertPlan();

	bool readlock = readLockZone && !_this.getReferenceUnsafeStaticCast()->isLockedByCurrentThread();

	//_this.getReferenceUnsafeStaticCast()->rlock(readlock);

	Zone* thisZone = _this.getReferenceUnsafeStaticCast();

	try {
		SortedVector<OctTreeEntry*> entryObjects;

		thisZone->rlock(readlock);

		regionTree->inRange(x, y, z, range, entryObjects);

		thisZone->runlock(readlock);

		for (int i = 0; i < entryObjects.size(); ++i) {
			ActiveArea* obj = static_cast<ActiveArea*>(entryObjects.getUnsafe(i));
			objects->put(obj);
		}
	}catch (...) {
//		_this.getReferenceUnsafeStaticCast()->runlock(readlock);

		throw;
	}

//	_this.getReferenceUnsafeStaticCast()->runlock(readlock);

	return objects->size();
}

void ZoneImplementation::updateActiveAreas(TangibleObject* tano) {
	
	Locker _alocker(tano->getContainerLock());
	auto areas = *tano->getActiveAreas();
	_alocker.release();

	const Vector3& worldPos = tano->getWorldPosition();
	Zone* managedRef = _this.getReferenceUnsafeStaticCast();

	bool readlock = !managedRef->isLockedByCurrentThread();
	managedRef->rlock(readlock);

	SortedVector<OctTreeEntry*> entryObjects;
	try {
		regionTree->inRange(worldPos.getX(), worldPos.getY(), worldPos.getZ(), entryObjects);
	} catch (...) {
		error("unexpeted error caught in void ZoneImplementation::updateActiveAreas(SceneObject* object) {");
	}
	managedRef->runlock(readlock);
	managedRef->unlock(!readlock);

	try {

		for (int x = areas.size(); 0 < x--;) {
			const auto& area = areas.get(x);

			if (!area->containsPoint(worldPos.getX(), worldPos.getY(), worldPos.getZ(), tano->getParentID())) {
				tano->dropActiveArea(area);

				area->enqueueExitEvent(tano);
			} else {
				tano->notifyPositionUpdate(area);															
			}
		}


		for (int x = entryObjects.size(); 0 < x--;) {
			const ManagedReference<ActiveArea*>& area = static_cast<ActiveArea*>(entryObjects.get(x));

			if (!tano->hasActiveArea(area)) {
				tano->addActiveArea(area);

				area->enqueueEnterEvent(tano);	
			}
		}

		if (creatureManager) {
			const auto& worldAreas = creatureManager->getWorldSpawnAreas();
			
			if (worldAreas) {
				for (int x = worldAreas->size(); 0 < x--;) {
					const auto& area = worldAreas->get(x);
					
						
					if (!tano->hasActiveArea(area)
					&& area->containsPoint(worldPos.getX(), worldPos.getY(), worldPos.getZ(), tano->getParentID())) {
						tano->addActiveArea(area);
						area->notifyEnter(tano);
					}
				}
			}
		}

	} catch (...) {
		error("unexpected exception caught in void ZoneImplementation::updateActiveAreas(SceneObject* object) {");
		managedRef->wlock(!readlock);
		throw;
	}

	managedRef->wlock(!readlock);
}

void ZoneImplementation::addSceneObject(SceneObject* object) {
	ManagedReference<SceneObject*> old = objectMap->put(object->getObjectID(), object);

	//Civic and commercial structures map registration will be handled by their city
	if (object->isStructureObject()) {
		StructureObject* structure = cast<StructureObject*>(object);
		if (structure->isCivicStructure() || structure->isCommercialStructure()) {
			return;
		}
	//Same thing for player city bank/mission terminals
	} else if (object->isTerminal()) {
		Terminal* terminal = cast<Terminal*>(object);
		ManagedReference<SceneObject*> controlledObject = terminal->getControlledObject();
		if (controlledObject != nullptr && controlledObject->isStructureObject()) {
			StructureObject* structure = controlledObject.castTo<StructureObject*>();
			if (structure->isCivicStructure())
				return;
		}
	} else if (old == nullptr && object->isAiAgent()) {
		spawnedAiAgents.increment();
	}

	registerObjectWithPlanetaryMap(object);
}

//TODO: Do we need to send out some type of update when this happens?
void ZoneImplementation::registerObjectWithPlanetaryMap(SceneObject* object) {
#ifndef WITH_STM
	Locker locker(mapLocations);
#endif
	mapLocations->transferObject(object);

	// If the object is a valid location for entertainer missions then add it
	// to the planet's mission map.
	if (objectIsValidPlanetaryMapPerformanceLocation(object)) {
		PlanetManager* planetManager = getPlanetManager();
		if (planetManager != nullptr) {
			planetManager->addPerformanceLocation(object);
		}
	}
}

void ZoneImplementation::unregisterObjectWithPlanetaryMap(SceneObject* object) {
#ifndef WITH_STM
	Locker locker(mapLocations);
#endif
	mapLocations->dropObject(object);

	// If the object is a valid location for entertainer missions then remove it
	// from the planet's mission map.
	if (objectIsValidPlanetaryMapPerformanceLocation(object)) {
		PlanetManager* planetManager = getPlanetManager();
		if (planetManager != nullptr) {
			planetManager->removePerformanceLocation(object);
		}
	}
}

bool ZoneImplementation::objectIsValidPlanetaryMapPerformanceLocation(SceneObject* object) {
	BuildingObject* building = object->asBuildingObject();
	if (building == nullptr) {
		return false;
	}

	bool hasPerformanceLocationCategory = false;

	const PlanetMapCategory* planetMapCategory = object->getPlanetMapCategory();
	if (planetMapCategory != nullptr) {
		String category = planetMapCategory->getName();
		if (category == "cantina" || category == "hotel") {
			hasPerformanceLocationCategory = true;
		}
	}

	if (!hasPerformanceLocationCategory) {
		planetMapCategory = object->getPlanetMapSubCategory();
		if (planetMapCategory != nullptr) {
			String subCategory = planetMapCategory->getName();
			if (subCategory == "guild_theater") {
				hasPerformanceLocationCategory = true;
			}
		}
	}

	if (hasPerformanceLocationCategory) {
		if (building->isPublicStructure()) {
			return true;
		}
	}

	return false;
}

bool ZoneImplementation::isObjectRegisteredWithPlanetaryMap(SceneObject* object) {
#ifndef WITH_STM
	Locker locker(mapLocations);
#endif
	return mapLocations->containsObject(object);
}

void ZoneImplementation::updatePlanetaryMapIcon(SceneObject* object, byte icon) {
#ifndef WITH_STM
	Locker locker(mapLocations);
#endif
	mapLocations->updateObjectsIcon(object, icon);
}

void ZoneImplementation::dropSceneObject(SceneObject* object)  {
	ManagedReference<SceneObject*> oldObject = objectMap->remove(object->getObjectID());

	unregisterObjectWithPlanetaryMap(object);

	if (oldObject != nullptr && oldObject->isAiAgent()) {
		spawnedAiAgents.decrement();
	}
}

void ZoneImplementation::sendMapLocationsTo(CreatureObject* player) {
	GetMapLocationsResponseMessage* gmlr = new GetMapLocationsResponseMessage(zoneName, mapLocations, player);
	player->sendMessage(gmlr);
}

Reference<SceneObject*> ZoneImplementation::getNearestPlanetaryObject(SceneObject* object, const String& mapObjectLocationType) {
	Reference<SceneObject*> planetaryObject = nullptr;

#ifndef WITH_STM
	ReadLocker rlocker(mapLocations);
#endif

	const SortedVector<MapLocationEntry>& sortedVector = mapLocations->getLocation(mapObjectLocationType);

	float distance = 16000.f;

	for (int i = 0; i < sortedVector.size(); ++i) {
		SceneObject* obj = sortedVector.getUnsafe(i).getObject();

		float objDistance = object->getDistanceTo(obj);

		if (objDistance < distance) {
			planetaryObject = obj;
			distance = objDistance;
		}
	}

	return planetaryObject;
}

SortedVector<ManagedReference<SceneObject*> > ZoneImplementation::getPlanetaryObjectList(const String& mapObjectLocationType) {
	SortedVector<ManagedReference<SceneObject*> > retVector;
	retVector.setNoDuplicateInsertPlan();

#ifndef WITH_STM
	ReadLocker rlocker(mapLocations);
#endif

	const SortedVector<MapLocationEntry>& entryVector = mapLocations->getLocation(mapObjectLocationType);

	for (int i = 0; i < entryVector.size(); ++i) {
		const MapLocationEntry& entry = entryVector.getUnsafe(i);
		retVector.put(entry.getObject());
	}

	return retVector;
}

float ZoneImplementation::getMinX() {
	return planetManager->getTerrainManager()->getMin();
}

float ZoneImplementation::getMaxX() {
	return planetManager->getTerrainManager()->getMax();
}

float ZoneImplementation::getMinY() {
	return planetManager->getTerrainManager()->getMin();
}

float ZoneImplementation::getMaxY() {
	return planetManager->getTerrainManager()->getMax();
}


float ZoneImplementation::getMinZ() {
	return planetManager->getTerrainManager()->getMin();
}

float ZoneImplementation::getMaxZ() {
	return planetManager->getTerrainManager()->getMax();
}

void ZoneImplementation::updateCityRegions() {
	bool log = cityRegionUpdateVector.size() > 0;
	info("scheduling updates for " + String::valueOf(cityRegionUpdateVector.size()) + " cities", log);

	bool forceRebuild = server->shouldDeleteNavAreas();

	for (int i = 0; i < cityRegionUpdateVector.size(); ++i) {
		CityRegion* city = cityRegionUpdateVector.get(i);

		Locker locker(city);

		Time* nextUpdateTime = city->getNextUpdateTime();
		int seconds = -1 * round(nextUpdateTime->miliDifference() / 1000.f);

		if (seconds < 0) //If the update occurred in the past, force an immediate update.
			seconds = 0;

		city->setRadius(city->getRadius());
		city->setLoaded();

		city->cleanupCitizens();
		//city->cleanupDuplicateCityStructures();

		city->rescheduleUpdateEvent(seconds);

		if (city->hasAssessmentPending()) {
			Time* nextAssessmentTime = city->getNextAssessmentTime();
			int seconds2 = -1 * round(nextAssessmentTime->miliDifference() / 1000.f);

			if (seconds2 < 0)
				seconds2 = 0;

			city->scheduleCitizenAssessment(seconds2);
		}

		city->createNavMesh(NavMeshManager::MeshQueue, forceRebuild);

		if (!city->isRegistered())
			continue;

		if (city->getRegionsCount() == 0)
			continue;

		Region* region = city->getRegion(0);

		unregisterObjectWithPlanetaryMap(region);
		registerObjectWithPlanetaryMap(region);

		for(int i = 0; i < city->getStructuresCount(); i++){
			ManagedReference<StructureObject*> structure = city->getCivicStructure(i);
			unregisterObjectWithPlanetaryMap(structure);
			registerObjectWithPlanetaryMap(structure);
		}

		for(int i = 0; i < city->getCommercialStructuresCount(); i++){
			ManagedReference<StructureObject*> structure = city->getCommercialStructure(i);
			unregisterObjectWithPlanetaryMap(structure);
			registerObjectWithPlanetaryMap(structure);
		}
	}

	cityRegionUpdateVector.removeAll();
}

bool ZoneImplementation::isWithinBoundaries(const Vector3& position) {
	//Remove 1/16th of the size to match client limits. NOTE: it has not been verified to work like this in the client.
	//Normal zone size is 8192, 1/16th of that is 512 resulting in 7680 as the boundary value.
	float maxX = getMaxX() * 15 / 16;
	float minX = getMinX() * 15 / 16;
	float maxY = getMaxY() * 15 / 16;
	float minY = getMinY() * 15 / 16;

	if (maxX >= position.getX() && minX <= position.getX() &&
			maxY >= position.getY() && minY <= position.getY()) {
		return true;
	} else {
		return false;
	}
}

float ZoneImplementation::getBoundingRadius() {
	return planetManager->getTerrainManager()->getMax();
}
