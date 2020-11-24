/*
 */

#ifndef SHIPDEEDTEMPLATE_H_
#define SHIPDEEDTEMPLATE_H_

#include "templates/tangible/DeedTemplate.h"

class ShipDeedTemplate : public DeedTemplate {
private:
	String controlDeviceObjectTemplate;

public:
	ShipDeedTemplate() {

	}

	~ShipDeedTemplate() {

	}

	void readObject(LuaObject* templateData) {
		DeedTemplate::readObject(templateData);

		controlDeviceObjectTemplate = templateData->getStringField("shipControlObject");
    }

	String getControlDeviceObjectTemplate()
	{
		return controlDeviceObjectTemplate;
	}
};


#endif /* SHIPDEEDTEMPLATE_H_ */
