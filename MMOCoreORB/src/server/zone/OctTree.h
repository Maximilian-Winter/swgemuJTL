/*
Copyright (C) 2020 <SWGEmu>. All rights reserved.
Distribution of this file for usage outside of Core3 is prohibited.
*/

#ifndef OCTTREE_H_
#define OCTTREE_H_


#include "system/lang.h"

#include "engine/log/Logger.h"

#include "server/zone/OctTreeEntry.h"

#include "OctTreeNode.h"

namespace server {
  namespace zone {

	class OctTree : public Object {
		Reference<OctTreeNode*> root;

		static bool logTree;

		mutable ReadWriteLock mutex;

	public:
		OctTree();
		OctTree(float minx, float miny, float minz, float maxx, float maxy, float maxz);

		~OctTree();

		Object* clone();
		Object* clone(void* object);

		void free() {
			TransactionalMemoryManager::instance()->destroy(this);
		}

		void setSize(float minx, float miny, float minz, float maxx, float maxy, float maxz);

		void insert(OctTreeEntry *obj);

		void remove(OctTreeEntry *obj);

		void removeAll();

		void inRange(OctTreeEntry *obj, float range);

		void safeInRange(OctTreeEntry* obj, float range);

		int inRange(float x, float y, float z, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const;
		int inRange(float x, float y, float z, SortedVector<OctTreeEntry*>& objects) const;

		int inRange(float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const;
		int inRange(float x, float y, float z, float range, SortedVector<OctTreeEntry*>& objects) const;

		bool update(OctTreeEntry *obj);

	private:
		void _insert(const Reference<OctTreeNode*>& node, OctTreeEntry *obj);
		bool _update(const Reference<OctTreeNode*>& node, OctTreeEntry *obj);

		void _inRange(const Reference<OctTreeNode*>& node, OctTreeEntry *obj, float range);
		int _inRange(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const;
		int _inRange(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<OctTreeEntry* >& objects) const;
		int _inRange(const Reference<OctTreeNode*>& node, float x, float y, float z, SortedVector<ManagedReference<OctTreeEntry*> >& objects) const;
		int _inRange(const Reference<OctTreeNode*>& node, float x, float y, float z, SortedVector<OctTreeEntry*>& objects) const;

		void copyObjects(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<ManagedReference<OctTreeEntry*> >& objects);
		void copyObjects(const Reference<OctTreeNode*>& node, float x, float y, float z, float range, SortedVector<OctTreeEntry*>& objects);

	public:
		static void setLogging(bool doLog) {
			logTree = doLog;
		}

		inline static bool doLog() {
			return logTree;
		}
	};
  } // namespace zone
} // namespace server

#endif /*OctTree_H_*/
