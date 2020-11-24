#ifndef HYPERSPACETRAVELPOINT_H_
#define HYPERSPACETRAVELPOINT_H_

#include <atomic>

#include "server/zone/objects/creature/CreatureObject.h"

using namespace server::zone::objects::creature;

class HyperspaceTravelPoint : public Object {
	String pointZone;
	String pointName;
	Vector3 hyperspaceVector;

public:
	HyperspaceTravelPoint(const String& zoneName) {
		pointZone = zoneName;
		hyperspaceVector.set(0.f, 0.f, 0.f);
	}

	HyperspaceTravelPoint(const String& zone, const String& name, Vector3 hypersapceVector) {
		pointZone = zone;
		pointName = name;
		hyperspaceVector = hypersapceVector;
	}

	HyperspaceTravelPoint(const HyperspaceTravelPoint& ptp) : Object() {
		pointZone = ptp.pointZone;
		pointName = ptp.pointName;
		hyperspaceVector = ptp.hyperspaceVector;
	}

	HyperspaceTravelPoint& operator= (const HyperspaceTravelPoint& ptp) {
		if (this == &ptp)
			return *this;

		pointZone = ptp.pointZone;
		pointName = ptp.pointName;
		hyperspaceVector = ptp.hyperspaceVector;

		return *this;
	}

	void readLuaObject(LuaObject* luaObject) {
        pointZone = luaObject->getStringField("zonename");
		pointName = luaObject->getStringField("name");
		hyperspaceVector.set(
				luaObject->getFloatField("x"),
				luaObject->getFloatField("y"),
				luaObject->getFloatField("z")
		);
	}

	void setPointName(const String& name){
		pointName = name;
	}

	inline const String& getPointZone() const {
		return pointZone;
	}

	inline const String& getPointName() const {
		return pointName;
	}

	inline float getHyperspacePositionX() const {
		return hyperspaceVector.getX();
	}

	inline float getHyperspacePositionY() const {
		return hyperspaceVector.getY();
	}

	inline float getHyperspacePositionZ() const {
		return hyperspaceVector.getZ();
	}

	inline Vector3 getHyperspacePosition() const {
		return hyperspaceVector;
	}

	/**
	 * Returns true if this point is has the same zone and name that is passed in.
	 */
	inline bool isPoint(const String& zoneName, const String& name) const {
		return (zoneName == pointZone && name == pointName);
	}


	String toString() const {
		StringBuffer buf;

		buf << "[PlanetTravelPoint 0x" + String::hexvalueOf((int64)this)
			<< " Zone = '" << pointZone
			<< "' Name = '" << pointName
			<< " Space: " << hyperspaceVector.toString()<< "]";


		return buf.toString();
	}
};

#endif /* PLANETTRAVELPOINT_H_ */
