--Copyright (C) 2010 <SWGEmu>


--This File is part of Core3.

--This program is free software; you can redistribute 
--it and/or modify it under the terms of the GNU Lesser 
--General Public License as published by the Free Software
--Foundation; either version 2 of the License, 
--or (at your option) any later version.

--This program is distributed in the hope that it will be useful, 
--but WITHOUT ANY WARRANTY; without even the implied warranty of 
--MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
--See the GNU Lesser General Public License for
--more details.

--You should have received a copy of the GNU Lesser General 
--Public License along with this program; if not, write to
--the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

--Linking Engine3 statically or dynamically with other modules 
--is making a combined work based on Engine3. 
--Thus, the terms and conditions of the GNU Lesser General Public License 
--cover the whole combination.

--In addition, as a special exception, the copyright holders of Engine3 
--give you permission to combine Engine3 program with free software 
--programs or libraries that are released under the GNU LGPL and with 
--code included in the standard release of Core3 under the GNU LGPL 
--license (or modified versions of such code, with unchanged license). 
--You may copy and distribute such a system following the terms of the 
--GNU LGPL for Engine3 and the licenses of the other code concerned, 
--provided that you include the source code of that other code when 
--and as the GNU LGPL requires distribution of source code.

--Note that people who make modified versions of Engine3 are not obligated 
--to grant this special exception for their modified versions; 
--it is their choice whether to do so. The GNU Lesser General Public License 
--gives permission to release a modified version without this exception; 
--this exception also makes it possible to release a modified version 


object_ship_nebulon = object_ship_shared_nebulon:new {

	name = "nebulon",
	slideFactor = 1,
	chassisHitpoints = 30000,
	chassisMass = 10000,
	reactor = { name = "rct_generic", hitpoints = 4869.983, armor = 96.127,},
	engine = { name = "eng_generic", hitpoints = 4820.414, armor = 98.78848, speed = 9.807441, pitch = 2.907051, roll = 0, yaw = 2.99829, acceleration = 1.930248, rollRate = 0, pitchRate = 2.914913, deceleration = 0.9852591, yawRate = 2.904445,},
	shield_0 = { name = "shd_generic", hitpoints = 4954.367, armor = 191.872, regen = 4.846699, front = 193.4337, back = 199.7664,},
	shield_1 = { name = "shd_generic", hitpoints = 4975.484, armor = 198.3498, regen = 4.902131, front = 195.3655, back = 198.47,},
	armor_0 = { name = "arm_generic", hitpoints = 4864.32, armor = 4882.47,},
	armor_1 = { name = "arm_generic", hitpoints = 4916.466, armor = 4788.276,},
	capacitor = { name = "cap_generic", hitpoints = 4806.581, armor = 0, rechargeRate = 66.5183, energy = 1706.356,},
	bridge = { name = "bdg_generic", hitpoints = 0, armor = 19.44116,},
	hangar = { name = "hgr_generic", hitpoints = 0, armor = 19.1554,},
	targeting_station = { name = "tst_generic", hitpoints = 0, armor = 19.21219,},
	weapon_0 = { name = "wpn_capitalship_turret_test", hitpoints = 193.199, armor = 195.1185, rate = 0.3395525, drain = 22.84831, maxDamage = 19.71223, shieldEfficiency = 0, minDamage = 9.773687, ammo = 0, ammo_type = 0, armorEfficiency = 0,},
	weapon_1 = { name = "wpn_capitalship_turret_test", hitpoints = 191.3585, armor = 192.1416, rate = 0.334125, drain = 22.78276, maxDamage = 19.11772, shieldEfficiency = 0, minDamage = 9.965345, ammo = 0, ammo_type = 0, armorEfficiency = 0,},
}

ObjectTemplates:addTemplate(object_ship_nebulon, "object/ship/nebulon.iff")
