#ifndef HYPERSPACETRAVELPOINTLIST_H_
#define HYPERSPACETRAVELPOINTLIST_H_

#include "HyperspaceTravelPoint.h"

class HyperspaceTravelPointList : public VectorMap<String, Reference<HyperspaceTravelPoint*> >, public ReadWriteLock {

public:

	HyperspaceTravelPointList() : VectorMap<String, Reference<HyperspaceTravelPoint*> >() {
		setNoDuplicateInsertPlan();
		setNullValue(nullptr);
	}

	Reference<HyperspaceTravelPoint*> get(int index) {
		ReadLocker guard(this);

		Reference<HyperspaceTravelPoint*> point = VectorMap<String, Reference<HyperspaceTravelPoint*> >::get(index);

		return point;
	}

	Reference<HyperspaceTravelPoint*> get(const String& name) {
		ReadLocker guard(this);

		Reference<HyperspaceTravelPoint*> point = VectorMap<String, Reference<HyperspaceTravelPoint*> >::get(name);

		return point;
	}

	void readLuaObject(LuaObject* luaObject) {
		wlock();

		if (!luaObject->isValidTable())
			return;

		for (int i = 1; i <= luaObject->getTableSize(); ++i) {
			lua_State* L = luaObject->getLuaState();
			lua_rawgeti(L, -1, i);

			LuaObject hyperspaceTravelPointEntry(L);

			Reference<HyperspaceTravelPoint*> ptp = new HyperspaceTravelPoint("test");
			ptp->readLuaObject(&hyperspaceTravelPointEntry);

			put(ptp->getPointName(), ptp);

			hyperspaceTravelPointEntry.pop();
		}
		unlock();
	}
};

#endif /* PLANETTRAVELPOINTLIST_H_ */