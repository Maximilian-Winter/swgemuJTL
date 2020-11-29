/*
Copyright (C) 2020 <SWGEmu>. All rights reserved.
Distribution of this file for usage outside of Core3 is prohibited.
 */

#include <math.h>

#include "server/zone/OctTreeEntry.h"

#include "OctTree.h"

#define NO_ENTRY_REF_COUNTING

using namespace server::zone;

OctTreeNode::OctTreeNode() {
	objects.setNoDuplicateInsertPlan();

	minX = 0;
	minY = 0;
    minZ = 0;
	maxX = 0;
	maxY = 0;
    maxZ = 0;

	dividerX = 0;
	dividerY = 0;
    dividerZ = 0;
}

OctTreeNode::OctTreeNode(float minx, float miny, float minz, float maxx, float maxy, float maxz, OctTreeNode *parent) {
	objects.setNoDuplicateInsertPlan();

	parentNode = parent;

	minX = minx;
	minY = miny;
    minZ = minz;
	maxX = maxx;
	maxY = maxy;
    maxZ = maxz;

	if (!validateNode() || minX > maxX || minY > maxY|| minZ > maxZ) {
		StringBuffer msg;
		msg << "[OctTree] invalid node in create - " << toStringData();
		Logger::console.error(msg);
	}

	dividerX = (minX + maxX) / 2;
	dividerY = (minY + maxY) / 2;
    dividerZ = (minZ + maxZ) / 2;
}

OctTreeNode::~OctTreeNode() {
	
}


void OctTreeNode::addObject(OctTreeEntry *obj) {
	if (OctTree::doLog())
		System::out << hex << "object [" << obj->getObjectID() <<  "] added to OctTree"
		<< toStringData() << "\n";

	if (!validateNode())
		System::out << "[OctTree] invalid node in addObject() - " << toStringData() << "\n";

	objects.put(obj);
	obj->setNode(this);
}

void OctTreeNode::removeObject(OctTreeEntry *obj) {
	if (!objects.drop(obj)) {
		System::out << hex << "object [" << obj->getObjectID() <<  "] not found on OctTree"
				<< toStringData() << "\n";
	} else {
		obj->setNode(nullptr);

		if (OctTree::doLog())
			System::out <<  hex << "object [" << obj->getObjectID() <<  "] removed OctTree"
			<< toStringData() << "\n";
	}
}

void OctTreeNode::removeObject(int index) {
	OctTreeEntry* obj = objects.remove(index);
	obj->setNode(nullptr);
}

bool OctTreeNode::testInside(OctTreeEntry* obj) const {
	float x = obj->getPositionX();
	float y = obj->getPositionY();
    float z = obj->getPositionZ();

	return x >= minX && x < maxX && y >= minY && y < maxY && z >= minZ && z < maxZ;
}

bool OctTreeNode::testInRange(float x, float y, float z, float range) const {
	bool insideX = (minX <= x) && (x < maxX);
	bool insideY = (minY <= y) && (y < maxY);
    bool insideZ = (minZ <= z) && (z < maxZ);

	if (insideX && insideY && insideZ)
		return true;

	bool closeenoughX = (fabs(minX - x) <= range || fabs(maxX - x) <= range);
	bool closeenoughY = (fabs(minY - y) <= range || fabs(maxY - y) <= range);
    bool closeenoughZ = (fabs(minZ - z) <= range || fabs(maxZ - z) <= range);

	if ((insideX || closeenoughX) && (insideY || closeenoughY) && (insideZ || closeenoughZ))
		return true;
	else
		return false;
}

void OctTreeNode::check () {
	Reference<OctTreeNode*> parentNode = this->parentNode.get();

	if (isEmpty() && !hasSubNodes() && parentNode != nullptr)
    {
		if (parentNode->bottomNwNode == this)
			parentNode->bottomNwNode = nullptr;
		else if (parentNode->bottomNeNode == this)
			parentNode->bottomNeNode = nullptr;
		else if (parentNode->bottomSwNode == this)
			parentNode->bottomSwNode = nullptr;
		else if (parentNode->bottomSeNode == this)
			parentNode->bottomSeNode = nullptr;
        else if (parentNode->topNwNode == this)
			parentNode->topNwNode = nullptr;
		else if (parentNode->topNeNode == this)
			parentNode->topNeNode = nullptr;
		else if (parentNode->topSwNode == this)
			parentNode->topSwNode = nullptr;
		else if (parentNode->topSeNode == this)
			parentNode->topSeNode = nullptr;


		if (OctTree::doLog())
			System::out << "deleteing node (" << this << ")\n";
	}
}

String OctTreeNode::toStringData() const {
	StringBuffer s;
	s << "Node " << this << " (" << (int) minX << ","
			<< (int) minY << "," << (int) minZ << "," << (int) maxX
            << "," << (int) maxY << "," << (int) maxZ
			<< ") [" << objects.size() << "]";

	return s.toString();
}


bool OctTree::logTree = false;

OctTree::OctTree() {
	root = nullptr;
}

OctTree::OctTree(float minx, float miny, float minz, float maxx, float maxy, float maxz) {
	root = new OctTreeNode(minx, miny, minz, maxx, maxy, maxz, nullptr);
}

OctTree::~OctTree() {
	//delete root;

	root = nullptr;
}

Object* OctTree::clone() {
	return ObjectCloner<OctTree>::clone(this);
}

Object* OctTree::clone(void* mem) {
	return TransactionalObjectCloner<OctTree>::clone(this);
}

void OctTree::setSize(float minx, float miny, float minz, float maxx, float maxy, float maxz) {
	//delete root;

	root = new OctTreeNode(minx, miny, minz, maxx, maxy, maxz, nullptr);
}

void OctTree::insert(OctTreeEntry *obj) {

	E3_ASSERT(obj->getParent() == nullptr);

	Locker locker(&mutex);

	try {
		if (OctTree::doLog()) {
			System::out << hex << "object [" << obj->getObjectID() <<  "] inserting\n";
			System::out << "(" << obj->getPositionX() << ", " << obj->getPositionY() << ")\n";
		}

		if (obj->getNode() != nullptr)
			remove(obj);

		_insert(root, obj);

		if (OctTree::doLog())
			System::out << hex << "object [" << obj->getObjectID() <<  "] finished inserting\n";
	} catch (Exception& e) {
		System::out << "[OctTree] error - " << e.getMessage() << "\n";
		e.printStackTrace();
	}
}

bool OctTree::update(OctTreeEntry *obj) {

	Locker locker(&mutex);

	try {
		if (OctTree::doLog()) {
			System::out << hex << "object [" << obj->getObjectID() <<  "] updating on node "
					<< obj->getNode()->toStringData() << " \n" << "(" << obj->getPositionX()
					<< ", " << obj->getPositionY() << ")\n";
		}

		auto node = obj->getNode();

		if (node == nullptr) {
#ifdef OUTPUTQTERRORS
			System::out << hex << "object [" << obj->getObjectID() <<  "] updating error\n";
#endif
			return false;
		}

		bool res = _update(node, obj);

		if (OctTree::doLog())
			System::out << hex << "object [" << obj->getObjectID() <<  "] finished updating\n";

		return res;
	} catch (Exception& e) {
		System::out << "[OctTree] error - " << e.getMessage() << "\n";
		e.printStackTrace();

		return false;
	}
}

void OctTree::inRange(OctTreeEntry *obj, float range) {

	ReadLocker locker(&mutex);

	CloseObjectsVector* closeObjects = obj->getCloseObjects();

	float rangesq = range * range;

	float x = obj->getPositionX();
	float y = obj->getPositionY();
    float z = obj->getPositionZ();

	float oldx = obj->getPreviousPositionX();
	float oldy = obj->getPreviousPositionY();
    float oldz = obj->getPreviousPositionZ();

	try {
		if (closeObjects != nullptr) {
			for (int i = 0; i < closeObjects->size(); i++) {
				OctTreeEntry* o = closeObjects->get(i);
				ManagedReference<OctTreeEntry*> objectToRemove = o;
				ManagedReference<OctTreeEntry*> rootParent = o->getRootParent();

				if (rootParent != nullptr)
					o = rootParent;

				if (o != obj) {
					float deltaX = x - o->getPositionX();
					float deltaY = y - o->getPositionY();
                    float deltaZ = z - o->getPositionZ();

					if (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ > rangesq) {
						float oldDeltaX = oldx - o->getPositionX();
						float oldDeltaY = oldy - o->getPositionY();
                        float oldDeltaZ = oldy - o->getPositionZ();

						if (oldDeltaX * oldDeltaX + oldDeltaY * oldDeltaY + oldDeltaZ * oldDeltaZ <= rangesq) {
							obj->removeInRangeObject(objectToRemove);

							CloseObjectsVector* objCloseObjects = objectToRemove->getCloseObjects();

							if (objCloseObjects != nullptr)
								objectToRemove->removeInRangeObject(obj);
						}
					}
				}
			}
		}

		_inRange(root, obj, range);

		if (OctTree::doLog()) {
			System::out << hex << "object [" << obj->getObjectID() <<  "] in range (";


			System::out << "\n";
		}

	} catch (Exception& e) {
		System::out << "[OctTree] " << e.getMessage() << "\n";
		e.printStackTrace();
	}
}

int OctTree::inRange(float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const {
	ReadLocker locker(&mutex);

	try {
		return _inRange(root, x, y, z, range, objects);
	} catch (Exception& e) {
		System::out << "[OctTree] " << e.getMessage() << "\n";
		e.printStackTrace();
	}

	return 0;
}

int OctTree::inRange(float x, float y, float z, float range,
		SortedVector<OctTreeEntry*>& objects) const {
	ReadLocker locker(&mutex);

	try {
		return _inRange(root, x, y, z, range, objects);
	} catch (Exception& e) {
		System::out << "[OctTree] " << e.getMessage() << "\n";
		e.printStackTrace();
	}

	return 0;
}

void OctTree::remove(OctTreeEntry *obj) {
	Locker locker(&mutex);

	if (OctTree::doLog())
		System::out << hex << "object [" << obj->getObjectID() <<  "] removing\n";

	auto node = obj->getNode();

	if (node != nullptr) {
		if (!node->validateNode()) {
			System::out << "[OctTree] " << obj->getObjectID() << " error on remove() - invalid Node"
					<< node->toStringData() << "\n";
		}

		node->removeObject(obj);

		node->check();
		obj->setNode(nullptr);
	} else {
		System::out << hex << "object [" << obj->getObjectID() <<  "] ERROR - removing the node\n";
		StackTrace::printStackTrace();
	}

	if (OctTree::doLog())
		System::out << hex << "object [" << obj->getObjectID() <<  "] finished removing\n";
}

void OctTree::removeAll() {
	Locker locker(&mutex);

	if (root != nullptr) {
		root = nullptr;
		//delete root;
	}
}

void OctTree::_insert(const Reference<OctTreeNode*>& node, OctTreeEntry *obj) {
	
	obj->clearBounding();

	if (!node->isEmpty() && !node->hasSubNodes()) {
		if ((node->maxX - node->minX <= 8) && (node->maxY - node->minY <= 8) && (node->maxZ - node->minZ <= 8)) {
			node->addObject(obj);
			return;
		}

		for (int i = node->objects.size() - 1; i >= 0; i--) {
			OctTreeEntry* existing = node->getObject(i);

			if (existing->isBounding())
				continue;

			node->removeObject(i);

			if (existing->isInBottomSWArea(node)) 
            {
				if (node->bottomSwNode == nullptr)
					node->bottomSwNode = new OctTreeNode(node->minX, node->minY, node->minZ, node->dividerX, node->dividerY, node->dividerZ, node);

				_insert(node->bottomSwNode, existing);
			}
            else if (existing->isInTopSWArea(node)) 
            {
				if (node->topSwNode == nullptr)
					node->topSwNode = new OctTreeNode(node->minX, node->minY, node->dividerZ, node->dividerX, node->dividerY, node->maxZ, node);

				_insert(node->topSwNode, existing);
			}  
            else if (existing->isInBottomSEArea(node))
            {
				if (node->bottomSeNode == nullptr)
					node->bottomSeNode = new OctTreeNode(node->dividerX, node->minY, node->minZ, node->maxX, node->dividerY, node->dividerZ, node);

				_insert(node->bottomSeNode, existing);
			} 
            else if (existing->isInTopSEArea(node))
            {
				if (node->topSeNode == nullptr)
					node->topSeNode = new OctTreeNode(node->dividerX, node->minY, node->dividerZ, node->maxX, node->dividerY, node->maxZ, node);

				_insert(node->topSeNode, existing);
			} 
            else if (existing->isInBottomNWArea(node)) 
            {
				if (node->bottomNwNode == nullptr)
					node->bottomNwNode = new OctTreeNode(node->minX, node->dividerY, node->minZ, node->dividerX, node->maxY, node->dividerZ, node);

				_insert(node->bottomNwNode, existing);
			} 
            else if (existing->isInTopNWArea(node)) 
            {
				if (node->topNwNode == nullptr)
					node->topNwNode = new OctTreeNode(node->minX, node->dividerY, node->dividerZ, node->dividerX, node->maxY, node->maxZ, node);

				_insert(node->topNwNode, existing);
			} 
           else if(existing->isInBottomArea(node))
            {
				if (node->bottomNeNode == nullptr)
					node->bottomNeNode = new OctTreeNode(node->dividerX, node->dividerY, node->minZ, node->maxX, node->maxY, node->dividerZ, node);

				_insert(node->bottomNeNode, existing);
				
			}
            else 
            {
	
				if (node->topNeNode == nullptr)
					node->topNeNode = new OctTreeNode(node->dividerX, node->dividerY, node->dividerZ, node->maxX, node->maxY, node->maxZ, node);

				_insert(node->topNeNode, existing);
			}
		}
	}

	if (obj->isInBottomArea(node) || obj->isInTopArea(node)) {
		obj->setBounding();
		node->addObject(obj);

		return;
	}

	if (node->hasSubNodes()) {
		if (obj->isInBottomSWArea(node)) 
        {
			if (node->bottomSwNode == nullptr)
				node->bottomSwNode = new OctTreeNode(node->minX, node->minY, node->minZ, node->dividerX, node->dividerY, node->dividerZ, node);

			_insert(node->bottomSwNode, obj);
		} 
        else if (obj->isInTopSWArea(node)) 
        {
			if (node->topSwNode == nullptr)
				node->topSwNode = new OctTreeNode(node->minX, node->minY, node->dividerZ, node->dividerX, node->dividerY, node->maxZ, node);

			_insert(node->topSwNode, obj);
		} 
        else if (obj->isInBottomSEArea(node)) 
        {
			if (node->bottomSeNode == nullptr)
				node->bottomSeNode = new OctTreeNode(node->dividerX, node->minY, node->minZ, node->maxX, node->dividerY, node->dividerZ, node);

			_insert(node->bottomSeNode, obj);
		} 
        else if (obj->isInTopSEArea(node)) 
        {
			if (node->topSeNode == nullptr)
				node->topSeNode = new OctTreeNode(node->dividerX, node->minY, node->dividerZ, node->maxX, node->dividerY, node->maxZ, node);

			_insert(node->topSeNode, obj);
		} 
        else if (obj->isInBottomNWArea(node)) 
        {
			if (node->bottomNwNode == nullptr)
				node->bottomNwNode = new OctTreeNode(node->minX, node->dividerY, node->minZ, node->dividerX, node->maxY, node->dividerZ, node);

			_insert(node->bottomNwNode, obj);
		}
        else if (obj->isInTopNWArea(node)) 
        {
			if (node->topNwNode == nullptr)
				node->topNwNode = new OctTreeNode(node->minX, node->dividerY, node->dividerZ, node->dividerX, node->maxY, node->maxZ, node);

			_insert(node->topNwNode, obj);
		}
        else if (obj->isInBottomArea(node)) 
        {
			
			if (node->bottomNeNode == nullptr)
				node->bottomNeNode = new OctTreeNode(node->dividerX, node->dividerY, node->minZ, node->maxX, node->maxY, node->dividerZ, node);

			_insert(node->bottomNeNode, obj);
		}
        else 
        {
			if (node->topNeNode == nullptr)
				node->topNeNode = new OctTreeNode(node->dividerX, node->dividerY, node->dividerZ, node->maxX, node->maxY, node->maxZ, node);

			_insert(node->topNeNode, obj);
		}

		return;
	}

	node->addObject(obj);
}

bool OctTree::_update(const Reference<OctTreeNode*>& node, OctTreeEntry *obj) {

	if (node->testInside(obj))
		return true;

	Reference<OctTreeNode*> cur = node->parentNode.get();
	while (cur != nullptr && !cur->testInside(obj))
		cur = cur->parentNode.get();

	remove(obj);

	if (cur != nullptr) {
		_insert(cur, obj);
	}
#ifdef OUTPUTQTERRORS
	else
		System::out << "[OctTree] error on update() - invalid Node\n";
#endif

	return cur != nullptr;
}

void OctTree::safeInRange(OctTreeEntry* obj, float range) {
	CloseObjectsVector* closeObjectsVector = obj->getCloseObjects();

#ifdef NO_ENTRY_REF_COUNTING
	SortedVector<OctTreeEntry*> closeObjectsCopy;
#else
	SortedVector<ManagedReference<OctTreeEntry*> > closeObjectsCopy;
#endif

	Locker objLocker(obj);

	if (closeObjectsVector != nullptr) {
		closeObjectsCopy.removeAll(closeObjectsVector->size(), 50);
		closeObjectsVector->safeCopyTo(closeObjectsCopy);
	}

	float rangesq = range * range;

	float x = obj->getPositionX();
	float y = obj->getPositionY();
    float z = obj->getPositionZ();

#ifdef NO_ENTRY_REF_COUNTING
	SortedVector<OctTreeEntry*> inRangeObjects(500, 250);
#else
	SortedVector<ManagedReference<OctTreeEntry*> > inRangeObjects(500, 250);
#endif

	ReadLocker locker(&mutex);

	copyObjects(root, x, y, z, range, inRangeObjects);

	locker.release();

	for (int i = 0; i < inRangeObjects.size(); ++i) {
		OctTreeEntry *o = inRangeObjects.getUnsafe(i);

		if (o != obj) {
			float deltaX = x - o->getPositionX();
			float deltaY = y - o->getPositionY();
            float deltaZ = z - o->getPositionZ();

			try {
				if (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ <= rangesq) {
					CloseObjectsVector* objCloseObjects = obj->getCloseObjects();

					if (objCloseObjects != nullptr)
						obj->addInRangeObject(o, false);

					CloseObjectsVector* oCloseObjects = o->getCloseObjects();

					if (oCloseObjects != nullptr)
						o->addInRangeObject(obj);
				}
			} catch (...) {
				System::out << "unreported exception caught in safeInRange()\n";
			}
		} else {
			if (obj->getCloseObjects() != nullptr)
				obj->addInRangeObject(obj, false);
		}
	}


}

void OctTree::copyObjects(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<ManagedReference<server::zone::OctTreeEntry*> >& objects) {

	for (int i = 0; i < node->objects.size(); ++i) {
		objects.add(node->objects.getUnsafe(i).get());
	}

	if (node->hasSubNodes()) {
		if (node->topNwNode != nullptr && node->topNwNode->testInRange(x, y, z, range))
			copyObjects(node->topNwNode, x, y, z, range, objects);
        if (node->bottomNwNode != nullptr && node->bottomNwNode->testInRange(x, y, z, range))
			copyObjects(node->bottomNwNode, x, y, z, range, objects);
		if (node->topNeNode != nullptr && node->topNeNode->testInRange(x, y, z, range))
			copyObjects(node->topNeNode, x, y, z, range, objects);
        if (node->bottomNeNode != nullptr && node->bottomNeNode->testInRange(x, y, z, range))
			copyObjects(node->bottomNeNode, x, y, z, range, objects);
		if (node->topSwNode != nullptr && node->topSwNode->testInRange(x, y, z, range))
			copyObjects(node->topSwNode, x, y, z, range, objects);
        if (node->bottomSwNode != nullptr && node->bottomSwNode->testInRange(x, y, z, range))
			copyObjects(node->bottomSwNode, x, y, z, range, objects);
		if (node->topSeNode != nullptr && node->topSeNode->testInRange(x, y, z, range))
			copyObjects(node->topSeNode, x, y, z, range, objects);
        if (node->bottomSeNode != nullptr && node->bottomSeNode->testInRange(x, y, z, range))
			copyObjects(node->bottomSeNode, x, y, z, range, objects);
	}
}

void OctTree::copyObjects(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<server::zone::OctTreeEntry*>& objects) {


	for (int i = 0; i < node->objects.size(); ++i) {
		objects.add(node->objects.getUnsafe(i).get());
	}

	if (node->hasSubNodes()) {
		if (node->topNwNode != nullptr && node->topNwNode->testInRange(x, y, z, range))
			copyObjects(node->topNwNode, x, y, z, range, objects);
        if (node->bottomNwNode != nullptr && node->bottomNwNode->testInRange(x, y, z, range))
			copyObjects(node->bottomNwNode, x, y, z, range, objects);
		if (node->topNeNode != nullptr && node->topNeNode->testInRange(x, y, z, range))
			copyObjects(node->topNeNode, x, y, z, range, objects);
        if (node->bottomNeNode != nullptr && node->bottomNeNode->testInRange(x, y, z, range))
			copyObjects(node->bottomNeNode, x, y, z, range, objects);
		if (node->topSwNode != nullptr && node->topSwNode->testInRange(x, y, z, range))
			copyObjects(node->topSwNode, x, y, z, range, objects);
        if (node->bottomSwNode != nullptr && node->bottomSwNode->testInRange(x, y, z, range))
			copyObjects(node->bottomSwNode, x, y, z, range, objects);
		if (node->topSeNode != nullptr && node->topSeNode->testInRange(x, y, z, range))
			copyObjects(node->topSeNode, x, y, z, range, objects);
        if (node->bottomSeNode != nullptr && node->bottomSeNode->testInRange(x, y, z, range))
			copyObjects(node->bottomSeNode, x, y, z, range, objects);
	}
}

void OctTree::_inRange(const Reference<OctTreeNode*>& node, OctTreeEntry *obj, float range) {
	Reference<OctTreeNode*> refNode = node;

	float rangesq = range * range;

	float x = obj->getPositionX();
	float y = obj->getPositionY();
    float z = obj->getPositionZ();

	float oldx = obj->getPreviousPositionX();
	float oldy = obj->getPreviousPositionY();
    float oldz = obj->getPreviousPositionZ();

	for (int i = 0; i < refNode->objects.size(); i++) {
		OctTreeEntry *o = refNode->objects.get(i);

		if (o != obj) {
			float deltaX = x - o->getPositionX();
			float deltaY = y - o->getPositionY();
            float deltaZ = z - o->getPositionZ(); 

			if (deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ <= rangesq) {
				CloseObjectsVector* objCloseObjects = obj->getCloseObjects();
				if (objCloseObjects != nullptr && !objCloseObjects->contains(o)) {
					obj->addInRangeObject(o, false);
					//obj->notifyInsert(o);
				}

				CloseObjectsVector* oCloseObjects = o->getCloseObjects();

				if (oCloseObjects != nullptr && !oCloseObjects->contains(obj)) {
					o->addInRangeObject(obj);
				} else
					o->notifyPositionUpdate(obj);

			} else {
				float oldDeltaX = oldx - o->getPositionX();
				float oldDeltaY = oldy - o->getPositionY();
                float oldDeltaZ = oldz - o->getPositionZ();

				if (oldDeltaX * oldDeltaX + oldDeltaY * oldDeltaY + oldDeltaZ * oldDeltaZ <= rangesq) {

					CloseObjectsVector* objCloseObjects = obj->getCloseObjects();
					if (objCloseObjects != nullptr)
						obj->removeInRangeObject(o);


					CloseObjectsVector* oCloseObjects = o->getCloseObjects();

					if (oCloseObjects != nullptr)
						o->removeInRangeObject(obj);
				}
			}
		} else {
			if (obj->getCloseObjects() != nullptr)
				obj->addInRangeObject(obj, false);
		}
	}

	if (refNode->hasSubNodes()) {
		if (refNode->bottomNwNode != nullptr && refNode->bottomNwNode->testInRange(x, y, z, range))
			_inRange(refNode->bottomNwNode, obj, range);
        if (refNode->topNwNode != nullptr && refNode->topNwNode->testInRange(x, y, z, range))
			_inRange(refNode->topNwNode, obj, range);
		if (refNode->bottomNeNode != nullptr && refNode->bottomNeNode->testInRange(x, y, z, range))
			_inRange(refNode->bottomNeNode, obj, range);
        if (refNode->topNeNode != nullptr && refNode->topNeNode->testInRange(x, y, z, range))
			_inRange(refNode->topNeNode, obj, range);
		if (refNode->bottomSwNode != nullptr && refNode->bottomSwNode->testInRange(x, y, z, range))
			_inRange(refNode->bottomSwNode, obj, range);
        if (refNode->topSwNode != nullptr && refNode->topSwNode->testInRange(x, y, z, range))
			_inRange(refNode->topSwNode, obj, range);
		if (refNode->bottomSeNode != nullptr && refNode->bottomSeNode->testInRange(x, y, z, range))
			_inRange(refNode->bottomSeNode, obj, range);
		if (refNode->topSeNode != nullptr && refNode->topSeNode->testInRange(x, y, z, range))
			_inRange(refNode->topSeNode, obj, range);
	}
}

int OctTree::inRange(float x, float y, float z, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const {
	ReadLocker locker(&mutex);

	try {
		return _inRange(root, x, y, z, objects);
	} catch (Exception& e) {
		System::out << "[OctTree] " << e.getMessage() << "\n";
		e.printStackTrace();
	}

	return 0;
}

int OctTree::inRange(float x, float y, float z, SortedVector<OctTreeEntry*>& objects) const {
	ReadLocker locker(&mutex);

	try {
		return _inRange(root, x, y, z, objects);
	} catch (Exception& e) {
		System::out << "[OctTree] " << e.getMessage() << "\n";
		e.printStackTrace();
	}

	return 0;
}

int OctTree::_inRange(const Reference<OctTreeNode*>& node, float x, float y, float z,
		SortedVector<ManagedReference<OctTreeEntry*> >& objects) const {
	int count = 0;

	for (int i = 0; i < node->objects.size(); i++) {
		OctTreeEntry *o = node->objects.get(i);

		if (o->containsPoint(x, y, z)) {
			++count;
			objects.put(o);
		}
	}

	if (node->hasSubNodes()) {
		if (node->bottomNwNode != nullptr && node->bottomNwNode->testInside(x, y, z))
			count += _inRange(node->bottomNwNode, x, y, z, objects);
        if (node->topNwNode != nullptr && node->topNwNode->testInside(x, y, z))
			count += _inRange(node->topNwNode, x, y, z, objects);
		if (node->bottomNeNode != nullptr && node->bottomNeNode->testInside(x, y, z))
			count += _inRange(node->bottomNeNode, x, y, z, objects);
        if (node->topNeNode != nullptr && node->topNeNode->testInside(x, y, z))
			count += _inRange(node->topNeNode, x, y, z, objects);
		if (node->bottomSwNode != nullptr && node->bottomSwNode->testInside(x, y, z))
			count += _inRange(node->bottomSwNode, x, y, z, objects);
        if (node->topSwNode != nullptr && node->topSwNode->testInside(x, y, z))
			count += _inRange(node->topSwNode, x, y, z, objects);
		if (node->bottomSeNode != nullptr && node->bottomSeNode->testInside(x, y, z))
			count += _inRange(node->bottomSeNode, x, y, z, objects);
        if (node->topSeNode != nullptr && node->topSeNode->testInside(x, y, z))
			count += _inRange(node->topSeNode, x, y, z, objects);
	}

	return count;
}

int OctTree::_inRange(const Reference<OctTreeNode*>& node, float x, float y, float z,
		SortedVector<OctTreeEntry* >& objects) const {
	int count = 0;

	for (int i = 0; i < node->objects.size(); i++) {
		OctTreeEntry *o = node->objects.get(i);

		if (o->containsPoint(x, y, z)) {
			++count;
			objects.put(o);
		}
	}

	if (node->hasSubNodes()) {
		if (node->bottomNwNode != nullptr && node->bottomNwNode->testInside(x, y, z))
			count += _inRange(node->bottomNwNode, x, y, z, objects);
        if (node->topNwNode != nullptr && node->topNwNode->testInside(x, y, z))
			count += _inRange(node->topNwNode, x, y, z, objects);
		if (node->bottomNeNode != nullptr && node->bottomNeNode->testInside(x, y, z))
			count += _inRange(node->bottomNeNode, x, y, z, objects);
        if (node->topNeNode != nullptr && node->topNeNode->testInside(x, y, z))
			count += _inRange(node->topNeNode, x, y, z, objects);
		if (node->bottomSwNode != nullptr && node->bottomSwNode->testInside(x, y, z))
			count += _inRange(node->bottomSwNode, x, y, z, objects);
        if (node->topSwNode != nullptr && node->topSwNode->testInside(x, y, z))
			count += _inRange(node->topSwNode, x, y, z, objects);
		if (node->bottomSeNode != nullptr && node->bottomSeNode->testInside(x, y, z))
			count += _inRange(node->bottomSeNode, x, y, z, objects);
        if (node->topSeNode != nullptr && node->topSeNode->testInside(x, y, z))
			count += _inRange(node->topSeNode, x, y, z, objects);
	}

	return count;
}

int OctTree::_inRange(const Reference<OctTreeNode*>& node, float x, float y, float z,float range,
		SortedVector<ManagedReference<OctTreeEntry*> >& objects) const {
	int count = 0;

	for (int i = 0; i < node->objects.size(); i++) {
		OctTreeEntry *o = node->objects.get(i);

		if (o->isInRange(x, y, z, range)) {
			++count;
			objects.put(o);
		}
	}

	if (node->hasSubNodes()) {
		if (node->bottomNwNode != nullptr && node->bottomNwNode->testInside(x, y, z))
			count += _inRange(node->bottomNwNode, x, y, z, objects);
        if (node->topNwNode != nullptr && node->topNwNode->testInside(x, y, z))
			count += _inRange(node->topNwNode, x, y, z, objects);
		if (node->bottomNeNode != nullptr && node->bottomNeNode->testInside(x, y, z))
			count += _inRange(node->bottomNeNode, x, y, z, objects);
        if (node->topNeNode != nullptr && node->topNeNode->testInside(x, y, z))
			count += _inRange(node->topNeNode, x, y, z, objects);
		if (node->bottomSwNode != nullptr && node->bottomSwNode->testInside(x, y, z))
			count += _inRange(node->bottomSwNode, x, y, z, objects);
        if (node->topSwNode != nullptr && node->topSwNode->testInside(x, y, z))
			count += _inRange(node->topSwNode, x, y, z, objects);
		if (node->bottomSeNode != nullptr && node->bottomSeNode->testInside(x, y, z))
			count += _inRange(node->bottomSeNode, x, y, z, objects);
        if (node->topSeNode != nullptr && node->topSeNode->testInside(x, y, z))
			count += _inRange(node->topSeNode, x, y, z, objects);
	}

	return count;
}

int OctTree::_inRange(const Reference<OctTreeNode*>& node, float x, float y, float z, float range,
		SortedVector<OctTreeEntry* >& objects) const {
	int count = 0;

	for (int i = 0; i < node->objects.size(); i++) {
		OctTreeEntry *o = node->objects.getUnsafe(i);

		if (o->isInRange(x, y, z, range)) {
			++count;
			objects.put(o);
		}
	}

	if (node->hasSubNodes()) {
		if (node->bottomNwNode != nullptr && node->bottomNwNode->testInside(x, y, z))
			count += _inRange(node->bottomNwNode, x, y, z, objects);
        if (node->topNwNode != nullptr && node->topNwNode->testInside(x, y, z))
			count += _inRange(node->topNwNode, x, y, z, objects);
		if (node->bottomNeNode != nullptr && node->bottomNeNode->testInside(x, y, z))
			count += _inRange(node->bottomNeNode, x, y, z, objects);
        if (node->topNeNode != nullptr && node->topNeNode->testInside(x, y, z))
			count += _inRange(node->topNeNode, x, y, z, objects);
		if (node->bottomSwNode != nullptr && node->bottomSwNode->testInside(x, y, z))
			count += _inRange(node->bottomSwNode, x, y, z, objects);
        if (node->topSwNode != nullptr && node->topSwNode->testInside(x, y, z))
			count += _inRange(node->topSwNode, x, y, z, objects);
		if (node->bottomSeNode != nullptr && node->bottomSeNode->testInside(x, y, z))
			count += _inRange(node->bottomSeNode, x, y, z, objects);
        if (node->topSeNode != nullptr && node->topSeNode->testInside(x, y, z))
			count += _inRange(node->topSeNode, x, y, z, objects);
	}

	return count;
}
