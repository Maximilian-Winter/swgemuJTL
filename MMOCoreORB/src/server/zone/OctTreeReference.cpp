
#include "OctTreeReference.h"

using namespace server::zone;

bool OctTreeReference::toBinaryStream(ObjectOutputStream* stream) {
	return true;
}

bool OctTreeReference::parseFromBinaryStream(ObjectInputStream* stream) {
	return true;
}

OctTree* OctTreeReference::operator=(OctTree* obj) {
#ifdef WITH_STM
	setObject(obj);
#else
	updateObject(obj);
#endif

	return obj;
}

