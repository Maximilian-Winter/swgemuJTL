#include "server/zone/managers/space/SpaceManager.h"




void SpaceManagerImplementation::initialize() {
	info("Loading space.");

	loadLuaConfig();
}


void SpaceManagerImplementation::loadLuaConfig() 
{
	Lua* lua = new Lua();
	lua->init();

	lua->runFile("scripts/managers/space_manager.lua");
	LuaObject luaObject = lua->getGlobalObject("Hyperspace");

     if (luaObject.isValidTable()) 
    {
        LuaObject hyperspaceTravelPointsTable = luaObject.getObjectField("hyperspaceTravelPoints");
        hyperspaceTravelPointList->readLuaObject(&hyperspaceTravelPointsTable);
        hyperspaceTravelPointsTable.pop();
    } 
    else 
    {
        warning("Configuration settings not found.");
    }

	luaObject.pop();

	delete lua;
	lua = nullptr;
}