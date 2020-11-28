/*
 * RectangularAreaShapeImplementation.cpp
 *
 *  Created on: 31/10/2012
 *      Author: loshult
 */

#include "server/zone/objects/area/areashapes/RectangularAreaShape.h"
#include "server/zone/objects/area/areashapes/RingAreaShape.h"
#include "engine/util/u3d/Segment.h"

bool RectangularAreaShapeImplementation::containsPoint(float x, float y, float z) const {
	return (x >= bblX) && (x <= burX) && (y >= bblY) && (y <= burY) && (z >= bblZ) && (z <= burZ)
			&& (x >= tblX) && (x <= turX) && (y >= tblY) && (y <= turY) && (z >= tblZ) && (z <= turZ);
}

bool RectangularAreaShapeImplementation::containsPoint(const Vector3& point) const {
	return containsPoint(point.getX(), point.getY(), point.getZ());
}

Vector3 RectangularAreaShapeImplementation::getRandomPosition() const {
	float width = getWidth();
	float height = getHeight();
	float length = getLength();
	int x = System::random(width);
	int y = System::random(height);
	int z = System::random(length);
	Vector3 position;

	position.set(bblX + x, bblZ + z, bblY + y);

	return position;
}

Vector3 RectangularAreaShapeImplementation::getRandomPosition(const Vector3& origin, float minDistance, float maxDistance) const {
	bool found = false;
	Vector3 position;
	int retries = 5;

	while (!found && retries-- > 0) {
		int distance = System::random((int)(maxDistance - minDistance)) + minDistance;
		int angle = System::random(360) * Math::DEG2RAD;
		position.set(origin.getX() + distance * Math::cos(angle), origin.getX() + distance, origin.getY() + distance * Math::sin(angle));

		found = containsPoint(position);
	}

	if (!found)
		return getRandomPosition();

	return position;
}

bool RectangularAreaShapeImplementation::intersectsWith(AreaShape* areaShape) const {
	if (areaShape->isRingAreaShape()) {
		auto ring = cast<RingAreaShape*>(areaShape);
		Vector3 center = ring->getAreaCenter();

		if (ring->getOuterRadius2() < center.squaredDistanceTo(getClosestPoint(center))) // wholly outside the ring
			return false;
		else if (ring->getInnerRadius2() > center.squaredDistanceTo(getFarthestPoint(center))) // wholly inside the ring's inner circle
			return false;
		else
			return true;
	} else
		return areaShape->containsPoint(getClosestPoint(areaShape->getAreaCenter()));
}

Vector3 RectangularAreaShapeImplementation::getClosestPoint(const Vector3& position) const {
	// Calculate corners.
	Vector3 topLeft, topRight, bottomLeft, bottomRight;
	topLeft.set(bblX, 0, burY);
	topRight.set(burX, 0, burY);
	bottomLeft.set(bblX, 0, bblY);
	bottomRight.set(burX, 0, bblY);

	// Find closest point on each side.
	Segment topSegment(topLeft, topRight);
	Segment rightSegment(topRight, bottomRight);
	Segment bottomSegment(bottomRight, bottomLeft);
	Segment leftSegment(bottomLeft, topLeft);

	Vector3 top, right, bottom, left;
	top = topSegment.getClosestPointTo(position);
	right = rightSegment.getClosestPointTo(position);
	bottom = bottomSegment.getClosestPointTo(position);
	left = leftSegment.getClosestPointTo(position);

	// Find the closes of the four side points.
	Vector3 point = top;
	if (point.distanceTo(position) > right.distanceTo(position)) {
		point = right;
	}

	if (point.distanceTo(position) > bottom.distanceTo(position)) {
		point = bottom;
	}

	if (point.distanceTo(position) > left.distanceTo(position)) {
		point = left;
	}

	return point;
}

Vector3 RectangularAreaShapeImplementation::getFarthestPoint(const Vector3& position) const {
	// Calculate corners.
	Vector3 topLeft, topRight, bottomLeft, bottomRight;
	topLeft.set(bblX, 0, burY);
	topRight.set(burX, 0, burY);
	bottomLeft.set(bblX, 0, bblY);
	bottomRight.set(burX, 0, bblY);

	// Find the farthest of the four corners.
	Vector3 point = topLeft;
	if (point.distanceTo(position) < topRight.distanceTo(position)) {
		point = topRight;
	}
	if (point.distanceTo(position) < bottomLeft.distanceTo(position)) {
		point = bottomLeft;
	}
	if (point.distanceTo(position) < bottomRight.distanceTo(position)) {
		point = bottomRight;
	}

	return point;
}
