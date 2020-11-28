/*
 * InRangeObjectsVector.h
 *
 *  Created on: Aug 24, 2015
 *      Author: TheAnswer
 */

#ifndef SRC_SERVER_ZONE_INRANGEOBJECTSVECTOR_H_
#define SRC_SERVER_ZONE_INRANGEOBJECTSVECTOR_H_

#include "engine/engine.h"

#include "server/zone/OctTreeEntry.h"

typedef SortedVector<server::zone::OctTreeEntry*> InRangeObjectsVector;

#endif /* SRC_SERVER_ZONE_INRANGEOBJECTSVECTOR_H_ */
