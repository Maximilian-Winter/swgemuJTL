/*
 * CollisionManager.cpp
 *
 *  Created on: 01/03/2011
 *      Author: victor
 */

#include "CollisionManager.h"
#include "server/zone/Zone.h"
#include "server/zone/objects/building/BuildingObject.h"
#include "server/zone/objects/cell/CellObject.h"
#include "templates/SharedObjectTemplate.h"
#include "templates/appearance/PortalLayout.h"
#include "templates/appearance/FloorMesh.h"
#include "templates/appearance/PathGraph.h"
#include "terrain/manager/TerrainManager.h"
#include "server/zone/managers/planet/PlanetManager.h"
#include "server/zone/objects/ship/ShipObject.h"
#include "templates/appearance/DetailAppearanceTemplate.h"
#include "templates/appearance/MeshAppearanceTemplate.h"

float CollisionManager::getRayOriginPoint(CreatureObject* creature) {
	float heightOrigin = creature->getHeight() - 0.3f;

	if (creature->isProne() || creature->isKnockedDown() || creature->isIncapacitated()) {
		heightOrigin = 0.3f;
	} else if (creature->isKneeling()) {
		heightOrigin /= 2.f;
	}

	return heightOrigin;
}

bool CollisionManager::checkLineOfSightInBuilding(SceneObject* object1, SceneObject* object2, SceneObject* building) {
	SharedObjectTemplate* objectTemplate = building->getObjectTemplate();
	const PortalLayout* portalLayout = objectTemplate->getPortalLayout();

	if (portalLayout == nullptr)
		return true;

	//we are in model space... in cells
	Vector3 rayOrigin = object1->getPosition();
	rayOrigin.set(rayOrigin.getX(), rayOrigin.getY(), rayOrigin.getZ() + 1.f);

	Vector3 rayEnd = object2->getPosition();
	rayEnd.set(rayEnd.getX(), rayEnd.getY(), rayEnd.getZ() + 1.f);

	Vector3 direction(Vector3(rayEnd - rayOrigin));
	direction.normalize();

	float distance = rayEnd.distanceTo(rayOrigin);
	float intersectionDistance;

	Ray ray(rayOrigin, direction);
	Triangle* triangle = nullptr;

	// we check interior cells
	for (int i = 1; i < portalLayout->getAppearanceTemplatesSize(); ++i) {
		const AppearanceTemplate *tmpl = portalLayout->getAppearanceTemplate(i);
		if(tmpl == nullptr)
			continue;

		if(tmpl->intersects(ray, distance, intersectionDistance, triangle, true))
			return false;
	}

	return true;
}
const AppearanceTemplate* CollisionManager::getCollisionAppearance(SceneObject* scno, int collisionBlockFlags) {
	SharedObjectTemplate* templateObject = scno->getObjectTemplate();

	if (templateObject == nullptr)
		return nullptr;

	if (!(templateObject->getCollisionActionBlockFlags() & collisionBlockFlags))
		return nullptr;

	const PortalLayout* portalLayout = templateObject->getPortalLayout();

	return (portalLayout != nullptr) ? portalLayout->getAppearanceTemplate(0) : templateObject->getAppearanceTemplate();
}

bool CollisionManager::checkSphereCollision(const Vector3& origin, float radius, Zone* zone) {
	Vector3 sphereOrigin(origin.getX(), origin.getZ(), origin.getY());

	SortedVector<ManagedReference<OctTreeEntry*> > objects(512, 512);
	zone->getInRangeObjects(origin.getX(), origin.getY(),origin.getZ(), 512, &objects, true);

	for (int i = 0; i < objects.size(); ++i) {

		SceneObject* scno = static_cast<SceneObject*>(objects.get(i).get());

		try {
			SharedObjectTemplate* templateObject = scno->getObjectTemplate();

			if (templateObject == nullptr)
				continue;

			if (!(templateObject->getCollisionActionBlockFlags() & 255))
				continue;

			Sphere sphere(convertToModelSpace(sphereOrigin, scno), radius);

			const PortalLayout* portalLayout = templateObject->getPortalLayout();
			if (portalLayout != nullptr) {
				if(portalLayout->getAppearanceTemplate(0)->testCollide(sphere))
					return true;
			} else {
				auto appearanceTemplate = templateObject->getAppearanceTemplate();

				if(appearanceTemplate && appearanceTemplate->testCollide(sphere))
					return true;
			}

		} catch (Exception& e) {

		} catch (...) {
			throw;
		}
	}

	return false;
}

bool CollisionManager::checkLineOfSightWorldToCell(const Vector3& rayOrigin, const Vector3& rayEnd, float distance, CellObject* cellObject) {
	ManagedReference<SceneObject*> building = cellObject->getParent().get();

	if (building == nullptr)
		return true;

	SharedObjectTemplate* objectTemplate = building->getObjectTemplate();
	const PortalLayout* portalLayout = objectTemplate->getPortalLayout();

	if (portalLayout == nullptr)
		return true;

	Ray ray = convertToModelSpace(rayOrigin, rayEnd, building);

	if (cellObject->getCellNumber() >= portalLayout->getAppearanceTemplatesSize())
		return true;

	const AppearanceTemplate* app = portalLayout->getAppearanceTemplate(cellObject->getCellNumber());
	float intersectionDistance;
	Triangle* triangle = nullptr;

	if (app->intersects(ray, distance, intersectionDistance, triangle, true))
		return false;

	return true;
}

bool CollisionManager::checkMovementCollision(CreatureObject* creature, float x, float z, float y, Zone* zone) {
	SortedVector<ManagedReference<OctTreeEntry*> > closeObjects;
	zone->getInRangeObjects(x, y, z, 128, &closeObjects, true);

	//Vector3 rayStart(x, z + 0.25, y);
	//Vector3 rayStart(creature->getWorldPositionX(), creature->getWorldPositionZ(), creature->getPos)
	Vector3 rayStart = creature->getWorldPosition();
	rayStart.set(rayStart.getX(), rayStart.getY(), rayStart.getZ() + 0.25f);
	//Vector3 rayEnd(x + System::random(512), z + 0.3f, y + System::random(512));

	/*Vector3 rayEnd;
	rayEnd.set(targetPosition.getX(), targetPosition.getY(), targetPosition.getZ());*/

	Vector3 rayEnd(x, z + 0.25, y);

	float maxDistance = rayEnd.distanceTo(rayStart);

	if (maxDistance == 0)
		return false;

	printf("%f\n", maxDistance);

	SortedVector<IntersectionResult> results;
	results.setAllowDuplicateInsertPlan();

	printf("starting test\n");

	Triangle* triangle;

	for (int i = 0; i < closeObjects.size(); ++i) {
		SceneObject* object = dynamic_cast<SceneObject*>(closeObjects.get(i).get());

		if (object == nullptr)
			continue;

		const AppearanceTemplate* appearance = getCollisionAppearance(object, 255);

		if (appearance == nullptr)
			continue;

		Ray ray = convertToModelSpace(rayStart, rayEnd, object);

		//results.removeAll(10, 10);

		//ordered by intersection distance
		//tree->intersects(ray, maxDistance, results);

		float intersectionDistance;

		if (appearance->intersects(ray, maxDistance, intersectionDistance, triangle, true)) {
			String str = object->getObjectTemplate()->getFullTemplateString();

			object->info("intersecting with me " + str, true);
			return true;
		}
	}

	return false;
}

Vector<float>* CollisionManager::getCellFloorCollision(float x, float y, CellObject* cellObject) {
	Vector<float>* collisions = nullptr;

	ManagedReference<SceneObject*> rootObject = cellObject->getRootParent();

	if (rootObject == nullptr)
		return nullptr;

	SharedObjectTemplate* templateObject = rootObject->getObjectTemplate();

	if (templateObject == nullptr)
		return nullptr;

	const PortalLayout* portalLayout = templateObject->getPortalLayout();

	if (portalLayout == nullptr)
		return nullptr;

	const FloorMesh* mesh = portalLayout->getFloorMesh(cellObject->getCellNumber());

	if (mesh == nullptr)
		return nullptr;

	const AABBTree* tree = mesh->getAABBTree();

	if (tree == nullptr)
		return nullptr;

	Vector3 rayStart(x, 16384.f, y);
	Vector3 rayEnd(x, -16384.f, y);

	Vector3 norm = rayEnd - rayStart;
	norm.normalize();

	Ray ray(rayStart, norm);

	SortedVector<IntersectionResult> results(3, 2);

	tree->intersects(ray, 16384 * 2, results);

	if (results.size() == 0)
		return nullptr;

	collisions = new Vector<float>(results.size(), 1);

	for (int i = 0; i < results.size(); ++i) {
		float floorHeight = 16384 - results.get(i).getIntersectionDistance();

		collisions->add(floorHeight);
	}

	return collisions;
}

float CollisionManager::getWorldFloorCollision(float x, float y, float z, Zone* zone, bool testWater) {
	PlanetManager* planetManager = zone->getPlanetManager();

	if (planetManager == nullptr)
		return 0.f;

	SortedVector<OctTreeEntry*> closeObjects;
	zone->getInRangeObjects(x, y, z, 128, &closeObjects, true, false);

	float height = 0;

	TerrainManager* terrainManager = planetManager->getTerrainManager();

	//need to include exclude affectors in the terrain calcs
	height = terrainManager->getHeight(x, y);

	if (z < height)
		return height;

	if (testWater) {
		float waterHeight;

		if (terrainManager->getWaterHeight(x, y, waterHeight))
			if (waterHeight > height)
				height = waterHeight;
	}

	Ray ray(Vector3(x, z+2.0f, y), Vector3(0, -1, 0));

	for (const auto& entry : closeObjects) {
		SceneObject* sceno = static_cast<SceneObject*>(entry);

		const AppearanceTemplate* app = getCollisionAppearance(sceno, 255);

		if (app != nullptr) {
			Ray rayModelSpace = convertToModelSpace(ray.getOrigin(), ray.getOrigin()+ray.getDirection(), sceno);

			IntersectionResults results;

			app->intersects(rayModelSpace, 16384 * 2, results);

			if (results.size()) { // results are ordered based on intersection distance from min to max
				float floorHeight = ray.getOrigin().getY() - results.getUnsafe(0).getIntersectionDistance();

				if (floorHeight > height)
					height = floorHeight;
			}
		} else {
			continue;
		}
	}

	return height;
}

float CollisionManager::getWorldFloorCollision(float x, float y, Zone* zone, bool testWater) {
	

	PlanetManager* planetManager = zone->getPlanetManager();

	if (planetManager == nullptr)
		return 0.f;

	float height = 0;

	TerrainManager* terrainManager = planetManager->getTerrainManager();

	//need to include exclude affectors in the terrain calcs
	height = terrainManager->getHeight(x, y);
	SortedVector<OctTreeEntry*> closeObjects;
	zone->getInRangeObjects(x, y,height, 128, &closeObjects, true, false);

	Vector3 rayStart(x, 16384.f, y);
	Vector3 rayEnd(x, -16384.f, y);

	if (testWater) {
		float waterHeight;

		if (terrainManager->getWaterHeight(x, y, waterHeight))
			if (waterHeight > height)
				height = waterHeight;
	}

	for (const auto& entry : closeObjects) {
		SceneObject* sceno = static_cast<SceneObject*>(entry);

		const AppearanceTemplate* app = getCollisionAppearance(sceno, 255);

		if (app != nullptr) {
			Ray ray = convertToModelSpace(rayStart, rayEnd, sceno);

			IntersectionResults results;

			app->intersects(ray, 16384 * 2, results);

			if (results.size()) { // results are ordered based on intersection distance from min to max
				float floorHeight = 16384.f - results.getUnsafe(0).getIntersectionDistance();

				if (floorHeight > height)
					height = floorHeight;
			}
		} else {
			continue;
		}
	}

	return height;
}

void CollisionManager::getWorldFloorCollisions(float x, float y, Zone* zone, SortedVector<IntersectionResult>* result, CloseObjectsVector* closeObjectsVector) {
	if (closeObjectsVector != nullptr) {
		Vector<OctTreeEntry*> closeObjects(closeObjectsVector->size(), 10);
		closeObjectsVector->safeCopyReceiversTo(closeObjects, CloseObjectsVector::COLLIDABLETYPE);

		getWorldFloorCollisions(x, y, zone, result, closeObjects);
	} else {
#ifdef COV_DEBUG
		zone->info("Null closeobjects vector in CollisionManager::getWorldFloorCollisions", true);
#endif
		SortedVector<ManagedReference<OctTreeEntry*> > closeObjects;

		zone->getInRangeObjects(x, y, zone->getHeight(x, y), 128, &closeObjects, true);

		getWorldFloorCollisions(x, y, zone, result, closeObjects);
	}
}

void CollisionManager::getWorldFloorCollisions(float x, float y, Zone* zone, SortedVector<IntersectionResult>* result, const SortedVector<ManagedReference<OctTreeEntry*> >& inRangeObjects) {
	Vector3 rayStart(x, 16384.f, y);
	Vector3 rayEnd(x, -16384.f, y);

	for (int i = 0; i < inRangeObjects.size(); ++i) {
		SceneObject* sceno = static_cast<SceneObject*>(inRangeObjects.get(i).get());

		const AppearanceTemplate* app = getCollisionAppearance(sceno, 255);
		if (app != nullptr) {
			Ray ray = convertToModelSpace(rayStart, rayEnd, sceno);

			app->intersects(ray, 16384 * 2, *result);
		}
	}
}

void CollisionManager::getWorldFloorCollisions(float x, float y, Zone* zone, SortedVector<IntersectionResult>* result, const Vector<OctTreeEntry*>& inRangeObjects) {
	Vector3 rayStart(x, 16384.f, y);
	Vector3 rayEnd(x, -16384.f, y);

	for (int i = 0; i < inRangeObjects.size(); ++i) {
		SceneObject* sceno = static_cast<SceneObject*>(inRangeObjects.get(i));
		const AppearanceTemplate* app = getCollisionAppearance(sceno, 255);
		if (app != nullptr) {
			Ray ray = convertToModelSpace(rayStart, rayEnd, sceno);

			app->intersects(ray, 16384 * 2, *result);
		}
	}
}


bool CollisionManager::checkLineOfSight(SceneObject* object1, SceneObject* object2) {
	Zone* zone = object1->getZone();

	if (zone == nullptr)
		return false;

	if (object2->getZone() != zone)
		return false;

	if (object1->isAiAgent() || object2->isAiAgent()) {
		//Vector<WorldCoordinates>* path = PathFinderManager::instance()->findPath(object1, object2, zone);

//		if (path == nullptr)
//			return false;
//		else
//			delete path;
	}

	ManagedReference<SceneObject*> rootParent1 = object1->getRootParent();
	ManagedReference<SceneObject*> rootParent2 = object2->getRootParent();

	if (rootParent1 != nullptr || rootParent2 != nullptr) {
		if (rootParent1 == rootParent2) {
			return CollisionManager::checkLineOfSightInBuilding(object1, object2, rootParent1);
		} else if (rootParent1 != nullptr && rootParent2 != nullptr)
			return false; //different buildings
	}

	//switching z<->y, adding player height (head)
	Vector3 rayOrigin = object1->getWorldPosition();

	float heightOrigin = 1.f;
	float heightEnd = 1.f;

	UniqueReference<SortedVector<OctTreeEntry*>* > closeObjectsNonReference;/* new SortedVector<OctTreeEntry* >();*/
	UniqueReference<SortedVector<ManagedReference<OctTreeEntry*> >*> closeObjects;/*new SortedVector<ManagedReference<OctTreeEntry*> >();*/

	int maxInRangeObjectCount = 0;

	if (object1->getCloseObjects() == nullptr) {
#ifdef COV_DEBUG
		object1->info("Null closeobjects vector in CollisionManager::checkLineOfSight for " + object1->getDisplayedName(), true);
#endif

		closeObjects = new SortedVector<ManagedReference<OctTreeEntry*> >();
		zone->getInRangeObjects(object1->getPositionX(), object1->getPositionY(), object1->getPositionZ(), 512, closeObjects, true);
	} else {
		closeObjectsNonReference = new SortedVector<OctTreeEntry* >();

		CloseObjectsVector* vec = (CloseObjectsVector*) object1->getCloseObjects();
		vec->safeCopyReceiversTo(*closeObjectsNonReference.get(), CloseObjectsVector::COLLIDABLETYPE);
	}

	if (object1->isCreatureObject())
		heightOrigin = getRayOriginPoint(object1->asCreatureObject());

	if (object2->isCreatureObject())
		heightEnd = getRayOriginPoint(object2->asCreatureObject());

	rayOrigin.set(rayOrigin.getX(), rayOrigin.getY(), rayOrigin.getZ() + heightOrigin);

	Vector3 rayEnd = object2->getWorldPosition();
	rayEnd.set(rayEnd.getX(), rayEnd.getY(), rayEnd.getZ() + heightEnd);

	float dist = rayEnd.distanceTo(rayOrigin);
	float intersectionDistance;
	Triangle* triangle = nullptr;

	try {
		for (int i = 0; i < (closeObjects != nullptr ? closeObjects->size() : closeObjectsNonReference->size()); ++i) {
			const AppearanceTemplate* app = nullptr;

			SceneObject* scno;

			if (closeObjects != nullptr) {
				scno = static_cast<SceneObject*>(closeObjects->get(i).get());
			} else {
				scno = static_cast<SceneObject*>(closeObjectsNonReference->get(i));
			}

			if (scno == object2)
				continue;

			try {
				app = getCollisionAppearance(scno, 255);

				if (app == nullptr)
					continue;

			} catch (const Exception& e) {
				app = nullptr;
			}

			if (app != nullptr) {
				//moving ray to model space
				Ray ray = convertToModelSpace(rayOrigin, rayEnd, scno);

				//structure->info("checking ray with building dir" + String::valueOf(structure->getDirectionAngle()), true);

				if (app->intersects(ray, dist, intersectionDistance, triangle, true)) {
					return false;
				}
			}
		}
	} catch (const Exception& e) {
		Logger::console.error("unreported exception caught in bool CollisionManager::checkLineOfSight(SceneObject* object1, SceneObject* object2) ");
		Logger::console.error(e.getMessage());
	}

//	zone->runlock();

	ManagedReference<SceneObject*> parent1 = object1->getParent().get();
	ManagedReference<SceneObject*> parent2 = object2->getParent().get();

	if (parent1 != nullptr || parent2 != nullptr) {
		CellObject* cell = nullptr;

		if (parent1 != nullptr && parent1->isCellObject()) {
			cell = cast<CellObject*>(parent1.get());
		} else if (parent2 != nullptr && parent2->isCellObject()) {
			cell = cast<CellObject*>(parent2.get());
		}

		if (cell != nullptr) {
			return checkLineOfSightWorldToCell(rayOrigin, rayEnd, dist, cell);
		}
	}

	return true;
}

const TriangleNode* CollisionManager::getTriangle(const Vector3& point, const FloorMesh* floor) {
	/*PathGraph* graph = node->getPathGraph();
	FloorMesh* floor = graph->getFloorMesh();*/

	const AABBTree* aabbTree = floor->getAABBTree();

	//Vector3 nodePosition = node->getPosition();

	Vector3 rayOrigin(point.getX(), point.getZ() + 0.5, point.getY());
	//Vector3 rayOrigin(point.getX(), point.getY(), point.getZ() + 0.2);
	Vector3 direction(0, -1, 0);

	Ray ray(rayOrigin, direction);

	float intersectionDistance = 0;
	Triangle* triangle = nullptr;

	aabbTree->intersects(ray, 4, intersectionDistance, triangle, true);

	if (triangle == nullptr) {
		//System::out << "CollisionManager::getTriangle triangleNode nullptr" << endl;

		return floor->findNearestTriangle(rayOrigin);
	}

	TriangleNode* triangleNode = static_cast<TriangleNode*>(triangle);

	return triangleNode;
}

Vector3 CollisionManager::convertToModelSpace(const Vector3& point, SceneObject* model) {
	Reference<Matrix4*> modelMatrix = getInverseTransform(model);

	Vector3 transformedPoint = point * *modelMatrix;

	return transformedPoint;
}

Matrix4 CollisionManager::Inverse(Matrix4 mat) {
	//
	// Inversion by Cramer's rule.  Code taken from an Intel publication
	//
	double Result[4][4];
	double tmp[12]; /* temp array for pairs */
	double src[16]; /* array of transpose source matrix */
	double det; /* determinant */
	/* transpose matrix */
	for (unsigned int i = 0; i < 4; i++) {
		src[i + 0] = mat[i][0];
		src[i + 4] = mat[i][1];
		src[i + 8] = mat[i][2];
		src[i + 12] = mat[i][3];
	}
	/* calculate pairs for first 8 elements (cofactors) */
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	/* calculate first 8 elements (cofactors) */
	Result[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	Result[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	Result[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	Result[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	Result[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	Result[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	Result[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	Result[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	Result[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	Result[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	Result[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	Result[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	Result[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	Result[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	Result[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	Result[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
	/* calculate pairs for second 8 elements (cofactors) */
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];

	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];
	/* calculate second 8 elements (cofactors) */
	Result[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	Result[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	Result[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	Result[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	Result[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	Result[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	Result[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	Result[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	Result[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	Result[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	Result[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	Result[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	Result[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	Result[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	Result[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	Result[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
	/* calculate determinant */
	det = src[0] * Result[0][0] + src[1] * Result[0][1] + src[2] * Result[0][2] + src[3] * Result[0][3];
	/* calculate matrix inverse */
	det = 1.0f / det;

	Matrix4 FloatResult;
	for (unsigned int i = 0; i < 4; i++) {
		for (unsigned int j = 0; j < 4; j++) {
			FloatResult[i][j] = float(Result[i][j] * det);
		}
	}
	return FloatResult;
}

Matrix4 CollisionManager::getTransformMatrix(SceneObject* model) {
	Matrix4 translationMatrix;
	translationMatrix.setRotationMatrix(model->getDirection()->toMatrix3());
	translationMatrix.transpose();
	translationMatrix.setTranslation(model->getPositionX(), model->getPositionZ(), model->getPositionY());
	return translationMatrix;
}

Reference<Matrix4*> CollisionManager::getInverseTransform(SceneObject *model) {
	//this can be optimized by storing the matrix on the model and update it when needed
	//Reference

	Reference<Matrix4*> modelMatrix = model->getTransformForCollisionMatrix();

	if (true) {//modelMatrix == nullptr) {
		//modelMatrix = new Matrix4();

		Matrix4 translationMatrix;
		translationMatrix.setRotationMatrix(model->getDirection()->toMatrix3());
		translationMatrix.transpose();
		translationMatrix.setTranslation(model->getPositionX(), model->getPositionZ(), model->getPositionY());

		modelMatrix = new Matrix4(translationMatrix.inverse());

		model->setTransformForCollisionMatrixIfNull(modelMatrix);
	}

	return modelMatrix;
}

Ray CollisionManager::convertToModelSpace(const Vector3& rayOrigin, const Vector3& rayEnd, SceneObject* model) {
	Reference<Matrix4*> modelMatrix = getInverseTransform(model);

	Vector3 transformedOrigin = rayOrigin * *modelMatrix;
	Vector3 transformedEnd = rayEnd * *modelMatrix;

	Vector3 norm = transformedEnd - transformedOrigin;
	norm.normalize();

	Ray ray(transformedOrigin, norm);

	return ray;
}

bool CollisionManager::checkShipCollision(ShipObject* ship, const Vector3& targetPosition, Vector3& collisionPoint) {
	Zone* zone = ship->getZone();

	if (zone == nullptr)
		return false;

	TerrainManager* terrainManager = zone->getPlanetManager()->getTerrainManager();

	if (terrainManager->getProceduralTerrainAppearance() != nullptr) {
		float height = terrainManager->getHeight(targetPosition.getX(), targetPosition.getY());

		float waterHeight = -16368.f;

		if (terrainManager->getWaterHeight(targetPosition.getY(), targetPosition.getY(), waterHeight))
			height = Math::max(waterHeight, height);

		if (height > targetPosition.getZ()) {
			collisionPoint = targetPosition;
			collisionPoint.setZ(height);
			//ship->info("colliding with terrain", true);
			return true;
		}
	}

	Vector3 rayOrigin = ship->getWorldPosition();

	rayOrigin.set(rayOrigin.getX(), rayOrigin.getY(), rayOrigin.getZ());

	Vector3 rayEnd;
	rayEnd.set(targetPosition.getX(), targetPosition.getY(), targetPosition.getZ());

	float dist = rayEnd.distanceTo(rayOrigin);
	float intersectionDistance;
	Triangle* triangle = nullptr;

	SortedVector<ManagedReference<OctTreeEntry*> > objects(512, 512);
	zone->getInRangeObjects(targetPosition.getX(), targetPosition.getY(), targetPosition.getZ(), 512, &objects, true, false);

	for (int i = 0; i < objects.size(); ++i) {
		const AppearanceTemplate *app = nullptr;

		SceneObject* scno = static_cast<SceneObject*>(objects.get(i).get());

		try {
			app = getCollisionAppearance(scno, -1);

			if (app == nullptr)
				continue;

		} catch (Exception& e) {
			app = nullptr;
		} catch (...) {
			throw;
		}

		if (app != nullptr) {
			//moving ray to model space

			try {
				Ray ray = convertToModelSpace(rayOrigin, rayEnd, scno);

				//structure->info("checking ray with building dir" + String::valueOf(structure->getDirectionAngle()), true);

				if (app->intersects(ray, dist, intersectionDistance, triangle, true)) {

					//rayOrigin.set(rayOrigin.getX(), rayOrigin.getY(), rayOrigin.getZ());
					Vector3 direction = rayEnd - rayOrigin;
					direction.normalize();
					//intersectionDistance -= 0.5f;

					collisionPoint.set(rayOrigin.getX() + (direction.getX() * intersectionDistance), rayOrigin.getY() + (direction.getY() * intersectionDistance), rayOrigin.getZ() + (direction.getZ() * intersectionDistance));
					//ship->info("colliding with building", true);

					return true;
				}
			} catch (Exception& e) {
				ship->error(e.getMessage());
			} catch (...) {
				throw;
			}


		}
	}

	return false;
}

bool CollisionManager::checkShipWeaponCollision(ShipObject* obj, const Vector3 startPosition, const Vector3& targetPosition, Vector3& collisionPoint, Vector<ManagedReference<SceneObject*> >& collidedObjects) {
	Zone* zone = obj->getZone();

	if (zone == NULL)
		return false;

	Vector3 rayOrigin = startPosition;
	Vector3 rayEnd = targetPosition;
	TerrainManager* terrainManager = zone->getPlanetManager()->getTerrainManager();

	float dist = rayEnd.distanceTo(rayOrigin);
	float intersectionDistance;
	Triangle* triangle = NULL;


	Vector3 center = startPosition - ((targetPosition - startPosition) * 0.5f);
	SortedVector<ManagedReference<OctTreeEntry*> > objects;
	//zone->getInRangeObjects(center.getX(), center.getY(), center.length()*2.5f, &objects, true);
	obj->getCloseObjects()->safeCopyTo(objects);
	for (int i = 0; i < objects.size(); ++i) {
		const AppearanceTemplate *app = NULL;

		SceneObject* scno = cast<SceneObject*>(objects.get(i).get());

        if (scno == obj)
            continue;

		ShipObject *ship = dynamic_cast<ShipObject*>(scno);
		if (ship == NULL) {
			continue;
		}

		try {
			app = ship->getObjectTemplate()->getAppearanceTemplate();

		} catch (Exception& e) {
			app = NULL;
		} catch (...) {
			throw;
		}

		if (app != NULL) {
			//moving ray to model space

			try {
				Ray ray = convertToModelSpace(rayOrigin, rayEnd, ship);
				if (app->intersects(ray, dist, intersectionDistance, triangle, true)) {
					Vector3 point = ray.getOrigin() + (ray.getDirection() * intersectionDistance);
					collisionPoint.set(point.getX(), point.getY(), point.getZ());
					Logger::console.info("Tri Center: " + triangle->getBarycenter().toString(), true);
					collidedObjects.add(scno);

					return true;
				}
			} catch (Exception& e) {
				ship->error(e.getMessage());
			} catch (...) {
				throw;
			}


		}
	}

	return false;
}

const PathNode* CollisionManager::findNearestPathNode(const TriangleNode* triangle, const FloorMesh* floor, const Vector3& finalTarget) {
	// this is overkill TODO: find something faster
	const PathGraph* graph = floor->getPathGraph();

	if (graph == nullptr)
		return nullptr;

	const Vector<PathNode*>* pathNodes = graph->getPathNodes();

	PathNode* returnNode = nullptr;
	float distance = 16000;
	Vector3 trianglePos(triangle->getBarycenter());
	//trianglePos.set(trianglePos.getX(), trianglePos.getY(), trianglePos.getZ());*/

	for (int i = 0; i < pathNodes->size(); ++i) {
		PathNode* node = pathNodes->get(i);

		const TriangleNode* triangleOfPathNode = getTriangle(node->getPosition(), floor);

		Vector<const Triangle*>* path = TriangulationAStarAlgorithm::search(trianglePos, triangleOfPathNode->getBarycenter(), triangle, triangleOfPathNode);

		if (path == nullptr)
			continue;
		else {
			delete path;

			float sqrDistance = node->getPosition().squaredDistanceTo(finalTarget);

			if (sqrDistance < distance) {
				distance = sqrDistance;
				returnNode = node;
			}
		}
	}

	return returnNode;
}

bool CollisionManager::checkLineOfSightInParentCell(SceneObject* object, Vector3& endPoint) {
	ManagedReference<SceneObject*> parent = object->getParent().get();

	if (parent == nullptr || !parent->isCellObject())
		return true;

	CellObject* cell = cast<CellObject*>( parent.get());

	SharedObjectTemplate* objectTemplate = parent->getRootParent()->getObjectTemplate();
	const PortalLayout* portalLayout = objectTemplate->getPortalLayout();
	const AppearanceTemplate* appearanceMesh = nullptr;

	if (portalLayout == nullptr)
		return true;

	try {
		appearanceMesh = portalLayout->getAppearanceTemplate(cell->getCellNumber());
	} catch (Exception& e) {
		return true;
	}

	if (appearanceMesh == nullptr) {
		//info("null appearance mesh ");
		return true;
	}

	//switching Y<->Z, adding 0.1 to account floor
	Vector3 startPoint = object->getPosition();
	startPoint.set(startPoint.getX(), startPoint.getY(), startPoint.getZ() + 0.1f);

	endPoint.set(endPoint.getX(), endPoint.getY(), endPoint.getZ() + 0.1f);

	Vector3 dir = endPoint - startPoint;
	dir.normalize();

	float distance = endPoint.distanceTo(startPoint);
	float intersectionDistance;

	Ray ray(startPoint, dir);

	Triangle* triangle = nullptr;

	//nothing in the middle
	if (appearanceMesh->intersects(ray, distance, intersectionDistance, triangle, true))
		return false;

	Ray ray2(endPoint, Vector3(0, -1, 0));

	//check if we are in the cell with dir (0, -1, 0)
	if (!appearanceMesh->intersects(ray2, 64000.f, intersectionDistance, triangle, true))
		return false;

	return true;
}
