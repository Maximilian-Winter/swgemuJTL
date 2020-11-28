/*
 Copyright (C) 2020 <SWGEmu>. All rights reserved.
 Distribution of this file for usage outside of Core3 is prohibited.
 */

#ifndef OCTTREENODE_H_
#define OCTTREENODE_H_

#include "system/lang.h"

#include "engine/stm/TransactionalReference.h"
#include "engine/stm/TransactionalMemoryManager.h"

namespace server {
namespace zone {

class OctTree;
class OctTreeEntry;
class OctTreeEntryImplementation;

class OctTreeNode: public Object {
	SortedVector<Reference<OctTreeEntry*> > objects;

	WeakReference<OctTreeNode*> parentNode;
	Reference<OctTreeNode*> bottomNwNode;
	Reference<OctTreeNode*> bottomNeNode;
	Reference<OctTreeNode*> bottomSwNode;
	Reference<OctTreeNode*> bottomSeNode;
	Reference<OctTreeNode*> topNwNode;
	Reference<OctTreeNode*> topNeNode;
	Reference<OctTreeNode*> topSwNode;
	Reference<OctTreeNode*> topSeNode;

	float minX, minY, minZ;
	float maxX, maxY, maxZ;

	float dividerX, dividerY, dividerZ;

public:
	OctTreeNode();
	OctTreeNode(float minx, float miny, float minz, float maxx, float maxy, float maxz,
			OctTreeNode *parent);

	~OctTreeNode();

	Object* clone() {
		return ObjectCloner<OctTreeNode>::clone(this);
	}

	Object* clone(void* object) {
		return TransactionalObjectCloner<OctTreeNode>::clone(this);
	}

	void free() {
		TransactionalMemoryManager::instance()->destroy(this);
	}

	// Add a object to this node
	void addObject(OctTreeEntry *obj);

	OctTreeEntry* getObject(int index) {
		return objects.get(index);
	}

	// Remove a object by GUID
	void removeObject(OctTreeEntry *obj);

	void removeObject(int index);

	// Approximative test if a circle with center in x,y and
	// given radius crosses this node.
	bool testInRange(float x, float y, float z, float range) const;

	// Check if this node makes any sense to exist
	void check();

	bool validateNode() const {
		if (minX > maxX || minY > maxY || minZ > maxZ)
			return false;
		else
			return true;
	}

	// Check if this node has any associated objects
	inline bool isEmpty() const {
		return objects.isEmpty();
	}

	// Check if this node has children nodes
	inline bool hasSubNodes() const {
		return bottomNwNode != nullptr || bottomNeNode != nullptr || bottomSwNode != nullptr || bottomSeNode != nullptr
		|| topNwNode != nullptr|| topNeNode != nullptr || topSwNode != nullptr || topSeNode != nullptr;
	}

	// Test if the point is inside this node
	inline bool testInside(float x, float y, float z) const {
		return x >= minX && x < maxX && y >= minY && y < maxY && z >= minZ && z < maxZ;
	}

	// Test if the object is inside this node
	bool testInside(OctTreeEntry* obj) const;

	String toStringData() const;

	friend class server::zone::OctTree;
	friend class server::zone::OctTreeEntryImplementation;
};

} // namespace server
} // namespace zone


#endif 
