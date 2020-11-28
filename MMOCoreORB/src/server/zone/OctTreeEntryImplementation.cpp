/*
Copyright (C) 2007 <SWGEmu>. All rights reserved.
Distribution of this file for usage outside of Core3 is prohibited.
*/

#include <autogen/server/zone/ZoneServer.h>
#include "server/zone/OctTreeEntry.h"

#include "server/zone/OctTreeNode.h"

OctTreeEntryImplementation::OctTreeEntryImplementation(OctTreeNode* n) {
	node = n;
	bounding = false;

	//visibilityRange = 128;

	closeobjects = nullptr;

	//closeobjects.setInsertPlan(SortedVector<OctTreeEntry*>::NO_DUPLICATE);

	radius = 0.5f;

	radiusX = 0.0f;

	areaType = 0;

	//	areaType:	0:DEFAULT	1:CICLE		2:RECTANGLE		3:RING		-1:GLOBAL
	//	radius  :	= radius	= radius 	= height		= outer		:inf
	//	radiusX :	= nil		= nil		= width			= inner		:inf
	receiverFlags = 0;
}

void OctTreeEntryImplementation::setNode(OctTreeNode* n) {
	node = n;
}

bool OctTreeEntryImplementation::containsPoint(float px, float py, float pz) const 
{
	if (!areaType)
		return (((px - getPositionX()) * (px - getPositionX())) + ((py - getPositionY()) * (py - getPositionY()) + ((pz - getPositionZ()) * (pz - getPositionZ()))) <= radius * radius );

	return containsPoint(px, py, pz, 0.5f);
}


bool OctTreeEntryImplementation::containsPoint(float px, float py, float pz, float range) const {
	switch (areaType)
	{
		case -1 : return globalContainsPoint(px, py, pz);

		case  1 : return circleContainsPoint(px, py, pz, range);

		case  2 : return rectangleContainsPoint(px, py, pz, range);

		case  3 : return ringContainsPoint(px, py, pz, range);

		default : return false;
	}
}

bool OctTreeEntryImplementation::globalContainsPoint(float px, float py, float pz) const {
	return px > -7680.f && px < 7680.f && py > -7680.f && py < 7680.f && pz > -7680.f && pz < 7680.f;
}

bool OctTreeEntryImplementation::circleContainsPoint(float px, float py, float pz, float range) const {
	float deltaX = px - getPositionX();
	float deltaY = py - getPositionY();
	float deltaZ = pz - getPositionZ();
	float deltaR = range + radius;

	return (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ) <= (deltaR * deltaR);
}

bool OctTreeEntryImplementation::rectangleContainsPoint(float px, float py, float pz, float range) const {
	float deltaX = range + (radiusX / 2.f);
	float deltaY = range + (radius / 2.f);

	return px > (getPositionX() - deltaX) && px < (getPositionX() + deltaX)
		&& py > (getPositionY() - deltaY) && py < (getPositionY() + deltaY)
		&& pz > (getPositionZ() - deltaY) && pz < (getPositionZ() + deltaY);
}

bool OctTreeEntryImplementation::ringContainsPoint(float px, float py, float pz, float range) const {
	float deltaX  = px - getPositionX();
	float deltaY  = py - getPositionY();
	float deltaZ  = pz - getPositionZ();
	float deltaR  = range + radius;
	float deltaRx = range > radiusX ? 0 : range - radiusX;

	return (deltaX * deltaX) + (deltaY * deltaY) +  (deltaZ * deltaZ)<= (deltaR * deltaR)
		&& (deltaX * deltaX) + (deltaY * deltaY) + (deltaZ * deltaZ) >= (deltaRx * deltaRx);
}

bool OctTreeEntryImplementation::isInBottomSWArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->dividerX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->dividerY &&
			coordinates.getPositionZ() >= node->minZ && coordinates.getPositionZ() < node->dividerZ;
}

bool OctTreeEntryImplementation::isInTopSWArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->dividerX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->dividerY &&
			coordinates.getPositionZ() >= node->dividerZ && coordinates.getPositionZ() < node->maxZ;
}

bool OctTreeEntryImplementation::isInBottomSEArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->dividerX && coordinates.getPositionX() < node->maxX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->dividerY &&
			coordinates.getPositionZ() >= node->minZ && coordinates.getPositionZ() < node->dividerZ;
}

bool OctTreeEntryImplementation::isInTopSEArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->dividerX && coordinates.getPositionX() < node->maxX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->dividerY &&
			coordinates.getPositionZ() >= node->dividerZ && coordinates.getPositionZ() < node->maxZ;
}

bool OctTreeEntryImplementation::isInBottomNWArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->dividerX &&
			coordinates.getPositionY() >= node->dividerY && coordinates.getPositionY() < node->maxY &&
			coordinates.getPositionZ() >= node->minZ && coordinates.getPositionZ() < node->dividerZ;
}

bool OctTreeEntryImplementation::isInTopNWArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->dividerX &&
			coordinates.getPositionY() >= node->dividerY && coordinates.getPositionY() < node->maxY &&
			coordinates.getPositionZ() >= node->dividerZ && coordinates.getPositionZ() < node->maxZ;
}

bool OctTreeEntryImplementation::isInBottomArea(OctTreeNode* node) const {
	return  coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->maxX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->maxY &&
			coordinates.getPositionZ() >= node->minZ && coordinates.getPositionZ() < node->dividerZ;
}

bool OctTreeEntryImplementation::isInTopArea(OctTreeNode* node) const {
	return coordinates.getPositionX() >= node->minX && coordinates.getPositionX() < node->maxX &&
			coordinates.getPositionY() >= node->minY && coordinates.getPositionY() < node->maxY &&
			coordinates.getPositionZ() >= node->dividerZ && coordinates.getPositionZ() < node->maxZ;;
}

uint64 OctTreeEntryImplementation::getObjectID() {
	return _this.getReferenceUnsafeStaticCast()->_getObjectID();
}

OctTreeEntry* OctTreeEntryImplementation::getRootParent() {
	ManagedReference<OctTreeEntry*> grandParent = getParent();
	ManagedReference<OctTreeEntry*> tempParent = nullptr;

	if (grandParent == nullptr)
		return nullptr;

	while ((tempParent = grandParent->getParent()) != nullptr)
		grandParent = tempParent;

	return grandParent;
}

OctTreeEntry* OctTreeEntryImplementation::getParentUnsafe() {
	return parent.getReferenceUnsafe();
}

OctTreeEntry* OctTreeEntryImplementation::getRootParentUnsafe() {
	OctTreeEntry* parent = this->parent.getReferenceUnsafe();

	if (parent == nullptr)
		return nullptr;

	OctTreeEntry* grandParent = parent;
	OctTreeEntry* temp = nullptr;

	while ((temp = grandParent->getParentUnsafe()) != nullptr)
		grandParent = temp;

	return grandParent;
}

int OctTreeEntry::compareTo(OctTreeEntry* obj) {
	if (getDirtyObjectID() < obj->getDirtyObjectID())
		return 1;
	else if (getDirtyObjectID() > obj->getDirtyObjectID())
		return -1;
	else
		return 0;
}

int OctTreeEntryImplementation::compareTo(OctTreeEntry* obj) {
	if (getObjectID() < obj->getObjectID())
		return 1;
	else if (getObjectID() > obj->getObjectID())
		return -1;
	else
		return 0;
}

uint64 OctTreeEntry::getDirtyObjectID() {
	return _getObjectID();
}

uint64 OctTreeEntry::getObjectID() {
	return _getObjectID();
}

uint64 OctTreeEntryImplementation::getDirtyObjectID() {
	return _this.getReferenceUnsafeStaticCast()->_getObjectID();
}

float OctTreeEntryImplementation::getOutOfRangeDistance() const {
	return ZoneServer::CLOSEOBJECTRANGE;
}