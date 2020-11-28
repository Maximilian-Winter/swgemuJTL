//
// Created by Victor Popovici on 7/2/17.
//

#include "CloseObjectsVector.h"

#include "server/zone/OctTreeEntry.h"

CloseObjectsVector::CloseObjectsVector() : messageReceivers() {
	objects.setNoDuplicateInsertPlan();

	messageReceivers.setNoDuplicateInsertPlan();
}

void CloseObjectsVector::safeCopyTo(Vector<ManagedReference<OctTreeEntry*> >& vec) const {
	vec.removeAll(size(), size() / 2);

	ReadLocker locker(&mutex);

	//vec.addAll(*this);
	for (int i = 0; i < objects.size(); ++i) {
		const auto& obj = objects.getUnsafe(i);

		vec.emplace(obj);
	}
}

SortedVector<ManagedReference<OctTreeEntry*> > CloseObjectsVector::getSafeCopy() const {
	ReadLocker locker(&mutex);

	SortedVector<ManagedReference<OctTreeEntry*> > copy;

	for (int i = 0; i < objects.size(); ++i) {
		const auto& obj = objects.getUnsafe(i);

		copy.emplace(obj.get());
	}

	return copy;
}

void CloseObjectsVector::safeCopyTo(Vector<OctTreeEntry*>& vec) const {
	vec.removeAll(size(), size() / 2);

	ReadLocker locker(&mutex);

	for (int i = 0; i < objects.size(); ++i) {
		vec.add(objects.getUnsafe(i).get());
	}
}

bool CloseObjectsVector::contains(const Reference<OctTreeEntry*>& o) const {
	ReadLocker locker(&mutex);

	bool ret = objects.find(o) != -1;

	return ret;
}

void CloseObjectsVector::removeAll(int newSize, int newIncrement) {
	Locker locker(&mutex);

	objects.removeAll(newSize, newIncrement);

	messageReceivers.removeAll(newSize, newIncrement);

	count = 0;
}

void CloseObjectsVector::dropReceiver(OctTreeEntry* entry) {
	uint32 receiverTypes = entry->registerToCloseObjectsReceivers();

	if (receiverTypes && messageReceivers.size()) {
		for (int i = 0; i < CloseObjectsVector::MAXTYPES / 2; ++i) {
			uint32 type = 1 << i;

			if (receiverTypes & type) {
				int idx = messageReceivers.find(type);

				if (idx != -1) {
					auto& receivers = messageReceivers.elementAt(idx).getValue();

					receivers.drop(entry);
				}
			}
		}
	}
}

Reference<OctTreeEntry*> CloseObjectsVector::remove(int index) {
	Locker locker(&mutex);

	const auto& ref = objects.get(index);

	dropReceiver(ref);

	auto obj = objects.remove(index);

	count = objects.size();

	return obj;
}

bool CloseObjectsVector::drop(const Reference<OctTreeEntry*>& o) {
	Locker locker(&mutex);

	dropReceiver(o);

	auto res = objects.drop(o);

	count = objects.size();

	return res;
}

void CloseObjectsVector::safeCopyReceiversTo(Vector<OctTreeEntry*>& vec, uint32 receiverType) const {
	ReadLocker locker(&mutex);

	int i = messageReceivers.find(receiverType);

	if (i != -1) {
		const auto& receivers = messageReceivers.elementAt(i).getValue();

		vec.removeAll(receivers.size(), receivers.size() / 2);

		vec.addAll(receivers);
	}
}

void CloseObjectsVector::safeCopyReceiversTo(Vector<ManagedReference<OctTreeEntry*> >& vec, uint32 receiverType) const {
	ReadLocker locker(&mutex);

	int i = messageReceivers.find(receiverType);

	if (i != -1) {
		const auto& receivers = messageReceivers.elementAt(i).getValue();

		vec.removeAll(receivers.size(), receivers.size() / 2);

		for (int i = 0; i < receivers.size(); ++i)
			vec.emplace(receivers.getUnsafe(i));
	}
}

void CloseObjectsVector::safeAppendReceiversTo(Vector<OctTreeEntry*>& vec, uint32 receiverType) const {
	ReadLocker locker(&mutex);

	int i = messageReceivers.find(receiverType);

	if (i != -1) {
		const auto& receivers = messageReceivers.elementAt(i).getValue();
		vec.addAll(receivers);
	}
}

void CloseObjectsVector::safeAppendReceiversTo(Vector<ManagedReference<OctTreeEntry*> >& vec, uint32 receiverType) const {
	ReadLocker locker(&mutex);

	int i = messageReceivers.find(receiverType);

	if (i != -1) {
		const auto& receivers = messageReceivers.elementAt(i).getValue();
		for (int i = 0; i < receivers.size(); ++i)
			vec.emplace(receivers.getUnsafe(i));
	}
}

const Reference<OctTreeEntry*>& CloseObjectsVector::get(int idx) const {
	return objects.get(idx);
}

void CloseObjectsVector::putReceiver(OctTreeEntry* entry, uint32 receiverTypes) {
	if (receiverTypes) {
		for (int i = 0; i < CloseObjectsVector::MAXTYPES / 2; ++i) {
			uint32 type = 1 << i;

			if (receiverTypes & type) {
				int idx = messageReceivers.find(type);

				if (idx != -1) {
					auto& receivers = messageReceivers.elementAt(idx).getValue();

					receivers.put(entry);
				} else {
					SortedVector<OctTreeEntry*> vec;
					vec.setNoDuplicateInsertPlan();

					vec.put(entry);

					messageReceivers.put(std::move(type), std::move(vec));
				}
			}
		}
	}
}

int CloseObjectsVector::put(const Reference<OctTreeEntry*>& o) {
	uint32 receiverTypes = o->registerToCloseObjectsReceivers();

	Locker locker(&mutex);

	putReceiver(o.get(), receiverTypes);

	auto res = objects.put(o);

	count = objects.size();

	return res;
}

int CloseObjectsVector::put(Reference<OctTreeEntry*>&& o) {
	uint32 receiverTypes = o->registerToCloseObjectsReceivers();

	Locker locker(&mutex);
	putReceiver(o.get(), receiverTypes);

	auto res = objects.put(std::move(o));

	count = objects.size();

	return res;
}