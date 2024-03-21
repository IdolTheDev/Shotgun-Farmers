#pragma once
#include "vec.h"
#include "vec2.h"
#include <string>
#include "il2cpp_resolver.hpp"
#include "Lists.hpp"
#include <intrin.h>

namespace Functions
{
#pragma region Game Functions

	CodeStage_AntiCheat_ObscuredTypes_ObscuredFloat_o ObscuredFloat_Implicit(float value) {
		CodeStage_AntiCheat_ObscuredTypes_ObscuredFloat_o (UNITY_CALLING_CONVENTION t)(float);
		return reinterpret_cast<decltype(t)>(sdk::GameAssembly + 0x1278BD0)(value);
	}

	CodeStage_AntiCheat_ObscuredTypes_ObscuredInt_o ObscuredInt_Implicit(int32_t value) {
		CodeStage_AntiCheat_ObscuredTypes_ObscuredInt_o(UNITY_CALLING_CONVENTION t)(int32_t);
		return reinterpret_cast<decltype(t)>(sdk::GameAssembly + 0x1279500)(value);
	}

	CodeStage_AntiCheat_ObscuredTypes_ObscuredBool_o ObscuredBool_Implicit(bool value) {
		CodeStage_AntiCheat_ObscuredTypes_ObscuredBool_o(UNITY_CALLING_CONVENTION t)(bool);
		return reinterpret_cast<decltype(t)>(sdk::GameAssembly + 0x1276010)(value);
	}

	// UnityEngine_Camera$$Get_Main
	Unity::CCamera* GetMainCamera()
	{
		Unity::CCamera* (UNITY_CALLING_CONVENTION get_main)();
		return reinterpret_cast<decltype(get_main)>(sdk::GameAssembly + 0x0)();
	}	
	
	void SetDeveloperConsoleVisible(bool value)
	{
		void (UNITY_CALLING_CONVENTION t)(bool);
		return reinterpret_cast<decltype(t)>(Offsets::setdeveloperConsoleVisible)(value);
	}

	UnityEngine_Shader_o* UnityEngine_Shader__Find(Unity::System_String* name)
	{
		UnityEngine_Shader_o* (UNITY_CALLING_CONVENTION t)(Unity::System_String*);
		return reinterpret_cast<decltype(t)>(Offsets::FindShader)(name);
	}
#pragma endregion

#pragma region Hooks
	void(UNITY_CALLING_CONVENTION weaponLateUpdate)(Weapon_o*);
	void weaponLateUpdate_h(Weapon_o* weapon)
	{
		auto WeaponStats = weapon->fields.stats;
		if (!WeaponStats)
			return weaponLateUpdate(weapon);

		WeaponStats->fields.damage = ObscuredFloat_Implicit(200.0f);		// One Shot Kills
		WeaponStats->fields.cameraKickUp = ObscuredFloat_Implicit(0.0f);	// No Recoil
		WeaponStats->fields.speedMultiplier = ObscuredFloat_Implicit(5.0f);
		WeaponStats->fields.fireCooldown = ObscuredFloat_Implicit(0.0f);
		WeaponStats->fields.attackDistance = ObscuredFloat_Implicit(500.0f); 
		WeaponStats->fields.falloffMinimumDistance = ObscuredFloat_Implicit(500.0f);
		WeaponStats->fields.pelletSpread = ObscuredFloat_Implicit(200.0f);
		WeaponStats->fields.maxSpread = ObscuredFloat_Implicit(200.0f);
		WeaponStats->fields.isAutomatic = ObscuredBool_Implicit(true);
		WeaponStats->fields.totalProjectiles = ObscuredInt_Implicit(5000);
		WeaponStats->fields.mustBeCharged = false;
		WeaponStats->fields.HasDamageFalloff = false;
		WeaponStats->fields.HasBulletDrop = false;

		auto MuzzleFlash = (Unity::CComponent*)WeaponStats->fields.muzzleFlashFPV;
		if (MuzzleFlash)
			MuzzleFlash->GetGameObject()->Destroy();

		auto MuzzleFlashPre = (Unity::CComponent*)WeaponStats->fields.muzzleFlashPrefab;
		if (MuzzleFlashPre)
			MuzzleFlashPre->GetGameObject()->Destroy();

		auto BulletTrailPre = (Unity::CComponent*)WeaponStats->fields.bulletTrailPrefab;
		if (BulletTrailPre)
			BulletTrailPre->GetGameObject()->Destroy();

		auto BulletTrail = (Unity::CComponent*)WeaponStats->fields.trailParticle;
		if (BulletTrail)
			BulletTrail->GetGameObject()->Destroy();

		return weaponLateUpdate(weapon);
	}

	int32_t(UNITY_CALLING_CONVENTION Money)();
	int32_t MoneyHook() {
		return (int32_t)100000;
	}

	System_String_o* (UNITY_CALLING_CONVENTION getHardwareIdentifier)(Rewired_Controller_o*);
	System_String_o* getHardwareIdentifier_h(Rewired_Controller_o* controller) {
		// auto HWID = (Unity::System_String*)getHardwareIdentifier(controller);

		auto HWID = IL2CPP::String::New(vars::savedHWID);

		printf("getHardwareIdentifier Called %s\n", HWID->ToString().c_str());

		return (System_String_o*)HWID;
	}

	System_String_o* (UNITY_CALLING_CONVENTION getDeviceUniqueIdentifier)();
	System_String_o* getDeviceUniqueIdentifier_h() {
		//auto HWID = (Unity::System_String*)getDeviceUniqueIdentifier();

		auto HWID = IL2CPP::String::New(vars::savedHWID);

		printf("getDeviceUniqueIdentifier Called %s\n", HWID->ToString().c_str());

		return (System_String_o*)HWID;
	}

	bool (UNITY_CALLING_CONVENTION isdebug)();
	bool isdebug_h() {
		return true;
	}
#pragma endregion

#pragma region Custom Functions
	bool SpoofSteamID(std::string ID) {
		auto PlatformMgr = Unity::GameObject::Find("Platform Manager");
		if (!PlatformMgr)
			return false;

		printf("Platform Manager Was Valid\n");

		auto UserProfile = (UserProfileManager_o*)PlatformMgr->GetComponent("UserProfileManager");
		if (!UserProfile)
			return false;

		printf("UserProfile Was Valid\n");

		UserProfile->fields.PLAJDJKHFDE = (System_String_o*)IL2CPP::String::New(ID);

		printf("Spoofed SteamID to %s\n", ID.c_str());

		return true;
	}

	bool worldtoscreen(Unity::Vector3 world, Vector2& screen)
	{
		Unity::CCamera* CameraMain = Unity::Camera::GetMain(); // Get The Main Camera
		if (!CameraMain) {
			return false;
		}

		//Unity::Vector3 buffer = WorldToScreenPoint(CameraMain, world, 2);

		Unity::Vector3 buffer = CameraMain->CallMethodSafe<Unity::Vector3>("WorldToScreenPoint", world, Unity::eye::mono); // Call the worldtoscren function using monoeye (2)

		if (buffer.x > vars::screen_size.x || buffer.y > vars::screen_size.y || buffer.x < 0 || buffer.y < 0 || buffer.z < 0) // check if point is on screen
		{
			return false;
		}

		if (buffer.z > 0.0f) // Check if point is in view
		{
			screen = Vector2(buffer.x, vars::screen_size.y - buffer.y);
		}

		if (screen.x > 0 || screen.y > 0) // Check if point is in view
		{
			return true;
		}
	}
#pragma endregion
}