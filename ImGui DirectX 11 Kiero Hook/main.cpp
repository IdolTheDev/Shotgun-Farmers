#include "includes.h"
#include "Rendering.hpp"
#include "Font.h"
#include "sdk.h"
#include "Menu.h"
#include "Functions.h"
#include "kiero/minhook/include/MinHook.h"
#include "il2cpp_resolver.hpp"
#include "Lists.hpp"
#include "Callback.hpp"
#include <Utils/VFunc.hpp>
#include <iostream>
#include <PaternScan.hpp>
#include <intrin.h>
#include <random>
#include <chrono>
#include <thread>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void CreateConsole() {
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	SetConsoleTitle("IL2CPP");
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
}

void initvars() {
	if (IL2CPP::Initialize(true)) {
		printf("[ DEBUG ] Il2Cpp initialized\n");
	}
	else {
		printf("[ DEBUG ] Il2Cpp initialize failed, quitting...");
		Sleep(300);
		exit(0);
	}
	sdk::Base = (uintptr_t)GetModuleHandleA(NULL);
	printf("[ DEBUG ] Base Address: 0x%llX\n", sdk::Base);
	sdk::GameAssembly = (uintptr_t)GetModuleHandleA("GameAssembly.dll");
	printf("[ DEBUG ] GameAssembly Base Address: 0x%llX\n", sdk::GameAssembly);
	sdk::UnityPlayer = (uintptr_t)GetModuleHandleA("UnityPlayer.dll");
	printf("[ DEBUG ] UnityPlayer Base Address: 0x%llX\n", sdk::UnityPlayer);
	printf("----------------------------------------------------------\n");
}

void Log(uintptr_t address, const char* name) {
	printf("[ LOG ] %s: 0x%llX\n", name, address);
}
#define DEBUG // undefine in release

bool find_sigs() {
	Unity::il2cppClass* WeaponClass = IL2CPP::Class::Find("Weapon");
	Unity::il2cppClass* UnityEngineShaderClass = IL2CPP::Class::Find("UnityEngine.Shader");
	Unity::il2cppClass* RewiredControllerClass = IL2CPP::Class::Find("Rewired.Controller");
	Unity::il2cppClass* SystemInfoClass = IL2CPP::Class::Find("UnityEngine.SystemInfo");
	Unity::il2cppClass* UnityDebugClass = IL2CPP::Class::Find("UnityEngine.Debug");

	Offsets::WeaponLateUpdate = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(WeaponClass, "LateUpdate");
	Offsets::FindShader = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(UnityEngineShaderClass, "Find");
	Offsets::GetHardwareIdentifier = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(RewiredControllerClass, "get_hardwareIdentifier");
	Offsets::GetDeviceUniqueIdentifier = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(SystemInfoClass, "get_deviceUniqueIdentifier");
	Offsets::IsDebugBuild = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(UnityDebugClass, "get_isDebugBuild");
	Offsets::setdeveloperConsoleVisible = (uintptr_t)IL2CPP::Class::Utils::GetMethodPointer(UnityDebugClass, "set_developerConsoleVisible");

#ifdef DEBUG
	Log(Offsets::WeaponLateUpdate - sdk::GameAssembly, "WeaponLateUpdate");
	Log(Offsets::FindShader - sdk::GameAssembly, "FindShader");
	Log(Offsets::GetHardwareIdentifier - sdk::GameAssembly, "GetHardwareIdentifier");
	Log(Offsets::GetDeviceUniqueIdentifier - sdk::GameAssembly, "GetDeviceUniqueIdentifier");
	Log(Offsets::IsDebugBuild - sdk::GameAssembly, "IsDebugBuild");
	Log(Offsets::setdeveloperConsoleVisible - sdk::GameAssembly, "setdeveloperConsoleVisible");
#endif 

	return true;
}

void EnableHooks() {
	if (MH_CreateHook(reinterpret_cast<LPVOID*>(Offsets::WeaponLateUpdate), &Functions::weaponLateUpdate_h, (LPVOID*)&Functions::weaponLateUpdate) == MH_OK)
	{
		MH_EnableHook(reinterpret_cast<LPVOID*>(Offsets::WeaponLateUpdate));
	}

	if (MH_CreateHook(reinterpret_cast<LPVOID*>(sdk::GameAssembly + 0x23F4D20), &Functions::MoneyHook, (LPVOID*)&Functions::Money) == MH_OK)
	{
		MH_EnableHook(reinterpret_cast<LPVOID*>(sdk::GameAssembly + 0x23F4D20));
	}

	if (MH_CreateHook(reinterpret_cast<LPVOID*>(Offsets::GetHardwareIdentifier), &Functions::getHardwareIdentifier_h, (LPVOID*)&Functions::getHardwareIdentifier) == MH_OK)
	{
		MH_EnableHook(reinterpret_cast<LPVOID*>(Offsets::GetHardwareIdentifier));
	}	
	
	if (MH_CreateHook(reinterpret_cast<LPVOID*>(Offsets::GetDeviceUniqueIdentifier), &Functions::getDeviceUniqueIdentifier_h, (LPVOID*)&Functions::getDeviceUniqueIdentifier) == MH_OK)
	{
		MH_EnableHook(reinterpret_cast<LPVOID*>(Offsets::GetDeviceUniqueIdentifier));
	}	
	
	if (MH_CreateHook(reinterpret_cast<LPVOID*>(Offsets::IsDebugBuild), &Functions::isdebug_h, (LPVOID*)&Functions::isdebug) == MH_OK)
	{
		MH_EnableHook(reinterpret_cast<LPVOID*>(Offsets::IsDebugBuild));
	}
}

void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	return CallWindowProcA(oWndProc, hWnd, uMsg, wParam, lParam);
}

bool init = false;

void CostumeUA()
{
	while (true)
	{
		if (!vars::initil2cpp)
			continue;
		void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
		auto list = Unity::Object::FindObjectsOfType<Unity::CComponent>("CostumeButton");
		for (int i = 0; i < list->m_uMaxLength; i++)
		{
			if (!list->operator[](i)) {
				continue;
			}

			auto CostumeDataComp = list->operator[](i)->GetMemberValue<Unity::CComponent*>("costumeData");
			if (CostumeDataComp)
			{
				CostumeDataComp->SetMemberValue("altStatRequirement", 4); // 4 should be kills?
				CostumeDataComp->SetMemberValue("altStatValue", 0);
			}
		}

		IL2CPP::Thread::Detach(m_pThisThread);
		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	}
}

void Player_Cache()
{
	while (true)
	{
		if (!vars::initil2cpp)
			continue;
		void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());
		LocalPlayer = NULL;
		PlayerList.clear();
		auto list = Unity::Object::FindObjectsOfType<Unity::CComponent>("UnityEngine.CharacterController");
		for (int i = 0; i < list->m_uMaxLength; i++)
		{
			if (!list->operator[](i)) {
				continue;
			}

			auto obj = list->operator[](i)->GetGameObject();

			if (!obj)
				continue;

			auto PlayerControllerComp = obj->GetComponent("PlayerVehicleController");

			if (!PlayerControllerComp)
				continue;

			auto PlrCtrlPtr = list->operator[](i)->CallMethodSafe<Unity::il2cppArray<Unity::CComponent*>*>("GetComponentsInChildren", IL2CPP::Class::GetSystemType(IL2CPP::Class::Find("PlayerController")));

			if (!PlrCtrlPtr)
				continue;

			auto plrController = (PlayerController_o*)PlrCtrlPtr->operator[](0);

			if (!plrController)
				continue;

			if (plrController->fields.LocalPlayer)
				LocalPlayer = obj;

			PlayerList.push_back(obj);
		}
			
		IL2CPP::Thread::Detach(m_pThisThread);
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
}

static DWORD lastShotTime = 0;
static DWORD lasttick = 0;

auto RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags) -> VOID
{
	auto vList = ImGui::GetBackgroundDrawList();
	vList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, rounding, rounding_corners_flags);
}
auto HealthBar(float x, float y, float w, float h, int phealth, ImColor col) -> VOID
{
	auto vList = ImGui::GetBackgroundDrawList();

	int healthValue = max(0, min(phealth, 100));

	int barColor = ImColor
	(min(510 * (100 - healthValue) / 100, 255), min(510 * healthValue / 100, 255), 25, 255);
	vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), col);
	RectFilled(x, y, x + w, y + (((float)h / 100.0f) * (float)phealth), barColor, 0.0f, 0);
}

bool steamIDSpoofed = false;

void renderloop()
{
	DWORD currentTime = GetTickCount64(); // Get current time in milliseconds

	if (!vars::initil2cpp)
		return;

	if (!steamIDSpoofed)
		steamIDSpoofed = Functions::SpoofSteamID("76561199093586691");	
	else
		Functions::SetDeveloperConsoleVisible(true);

	if (!LocalPlayer)
		return;

	//auto PlrCtrlPtr = LocalPlayer->CallMethodSafe<Unity::il2cppArray<Unity::CComponent*>*>("GetComponentsInChildren", IL2CPP::Class::GetSystemType(IL2CPP::Class::Find("PlayerController")));

	//if (!PlrCtrlPtr)
	//	return;

	//auto LPController = (PlayerController_o*)PlrCtrlPtr->operator[](0);

	//if (!LPController)
	//	return;

	if (vars::fov_changer)
	{
		Unity::CCamera* CameraMain = Unity::Camera::GetMain();
		if (CameraMain != nullptr) {
			CameraMain->CallMethodSafe<void*>("set_fieldOfView", vars::CameraFOV);
		}
	}

	if (vars::crosshair)
	{
		ImColor coltouse = vars::CrossColor;
		if (vars::crosshairRainbow)
		{
			coltouse = ImColor(vars::Rainbow.x, vars::Rainbow.y, vars::Rainbow.z);
		}
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(vars::screen_center.x - vars::crosshairsize, vars::screen_center.y), ImVec2((vars::screen_center.x - vars::crosshairsize) + (vars::crosshairsize * 2), vars::screen_center.y), coltouse, 1.2f);
		ImGui::GetForegroundDrawList()->AddLine(ImVec2(vars::screen_center.x, vars::screen_center.y - vars::crosshairsize), ImVec2(vars::screen_center.x, (vars::screen_center.y - vars::crosshairsize) + (vars::crosshairsize * 2)), coltouse, 1.2f);
	}

	if (vars::fov_check)
	{
		ImGui::GetForegroundDrawList()->AddCircle(ImVec2(vars::screen_center.x, vars::screen_center.y), vars::aim_fov, ImColor(255, 255, 255), 360);
	}

	if (PlayerList.size() > 0)
	{
		for (int i = 0; i < PlayerList.size(); i++)
		{
			if (!PlayerList[i]) // Verify that the player is valid
				continue;

			if (PlayerList[i] == LocalPlayer)
				continue;

			auto plrTrans = PlayerList[i]->GetTransform();

			if (!plrTrans)
				continue;

			auto plrPos = plrTrans->GetPosition();

			if (vars::PlayerChams)
			{
				auto RenderList = PlayerList[i]->CallMethodSafe<Unity::il2cppArray<Unity::CComponent*>*>("GetComponentsInChildren", IL2CPP::Class::GetSystemType(IL2CPP::Class::Find("UnityEngine.Renderer")));

				if (!RenderList)
					continue;

				for (int t = 0; t < RenderList->m_uMaxLength; t++)
				{
					if (!RenderList->operator[](t))
						continue;

					if (!RenderList->operator[](t)->GetPropertyValue<bool>("enabled"))
						continue;

					auto Material = RenderList->operator[](t)->CallMethodSafe<Unity::CComponent*>("GetMaterial");

					if (!Material)
						continue;

					Material->CallMethodSafe<void*>("set_shader", vars::ChamsShader);
					Material->CallMethodSafe<void*>("SetInt", IL2CPP::String::New("_Cull"), 0);
					Material->CallMethodSafe<void*>("SetInt", IL2CPP::String::New("_ZTest"), 8);
					Material->CallMethodSafe<void*>("SetInt", IL2CPP::String::New("_ZWrite"), 0);
					Material->SetPropertyValue<Unity::Color>("color", Unity::Color(255, 0, 0));
				}
			}

			if (vars::PlayerSkeleton)
			{
				auto Joints = PlayerList[i]->CallMethodSafe<Unity::il2cppArray<Unity::CComponent*>*>("GetComponentsInChildren", IL2CPP::Class::GetSystemType(IL2CPP::Class::Find("ActorJoint")));
				if (Joints)
				{

					for (std::pair<int, int> bone_pair : sdk::bone_pairs)
					{
						if (!Joints->operator[](bone_pair.first) || !Joints->operator[](bone_pair.second))
							continue;

						auto j1 = Joints->operator[](bone_pair.first);
						auto j2 = Joints->operator[](bone_pair.second);

						if (!j1 || !j2)
							continue;

						auto j1T = j1->GetTransform();
						auto j2T = j2->GetTransform();

						if (!j1T || !j2T)
							continue;

						auto startpos = j1T->GetPosition();
						auto endpos = j2T->GetPosition();

						Vector2 start, end;

						if (Functions::worldtoscreen(startpos, start) && Functions::worldtoscreen(endpos, end))
						{
							ImGui::GetBackgroundDrawList()->AddLine(ImVec2(start.x, start.y), ImVec2(end.x, end.y), ImColor(255, 255, 255), 1.5f);
						}
					}
				}
			}

			if (vars::PlayerSnaplines)
			{
				Unity::Vector3 root_pos = plrPos;
				root_pos.y -= 0.2f;
				Vector2 pos;
				if (Functions::worldtoscreen(root_pos, pos)) {
					ImColor Colortouse = vars::PlayerSnaplineColor;
					if (vars::SnaplineRainbow)
						Colortouse = ImColor(vars::Rainbow.x, vars::Rainbow.y, vars::Rainbow.z);
					switch (vars::linetype)
					{
					case 1:
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vars::screen_center.x, vars::screen_size.y), ImVec2(pos.x, pos.y), Colortouse, 1.5f);
						break;
					case 2:
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(vars::screen_center.x, vars::screen_center.y), ImVec2(pos.x, pos.y), Colortouse, 1.5f);
						break;
					}
				}
			}
		}
	}

	if (currentTime - lasttick > 5) //  5 ms Timer For Whatever you want to do
	{	
		
		lasttick = currentTime;
	}
}

HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	void* m_pThisThread = IL2CPP::Thread::Attach(IL2CPP::Domain::Get());

	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			ImGui::GetIO().Fonts->AddFontDefault();
			ImFontConfig font_cfg;
			font_cfg.GlyphExtraSpacing.x = 1.2;
			gameFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(TTSquaresCondensedBold, 14, 14, &font_cfg);
			ImGui::GetIO().Fonts->AddFontDefault();
			vars::ChamsShader = Functions::UnityEngine_Shader__Find(IL2CPP::String::New("Hidden/Internal-Colored"));
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	pContext->RSGetViewports(&vars::vps, &vars::viewport);
	vars::screen_size = { vars::viewport.Width, vars::viewport.Height };
	vars::screen_center = { vars::viewport.Width / 2.0f, vars::viewport.Height / 2.0f };

	auto begin_scene = [&]() {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	};

	auto end_scene = [&]() {
		ImGui::Render();
	};

	begin_scene();

	auto isFrames = ImGui::GetFrameCount();
	static float isRed = 0.0f, isGreen = 0.01f, isBlue = 0.0f;
	if (isFrames % 1 == 0)
	{
		if (isGreen == 0.01f && isBlue == 0.0f)
		{
			isRed += 0.01f;

		}
		if (isRed > 0.99f && isBlue == 0.0f)
		{
			isRed = 1.0f;

			isGreen += 0.01f;

		}
		if (isGreen > 0.99f && isBlue == 0.0f)
		{
			isGreen = 1.0f;

			isRed -= 0.01f;

		}
		if (isRed < 0.01f && isGreen == 1.0f)
		{
			isRed = 0.0f;

			isBlue += 0.01f;

		}
		if (isBlue > 0.99f && isRed == 0.0f)
		{
			isBlue = 1.0f;

			isGreen -= 0.01f;

		}
		if (isGreen < 0.01f && isBlue == 1.0f)
		{
			isGreen = 0.0f;

			isRed += 0.01f;

		}
		if (isRed > 0.99f && isGreen == 0.0f)
		{
			isRed = 1.0f;

			isBlue -= 0.01f;

		}
		if (isBlue < 0.01f && isGreen == 0.0f)
		{
			isBlue = 0.0f;

			isRed -= 0.01f;

			if (isRed < 0.01f)
				isGreen = 0.01f;

		}
	}
	vars::Rainbow = ImVec4(isRed, isGreen, isBlue, 1.0f);

	if (vars::watermark)
	{
		render::DrawOutlinedText(gameFont, ImVec2(vars::screen_center.x, vars::screen_size.y - 20), 13.0f, ImColor(vars::Rainbow.x, vars::Rainbow.y, vars::Rainbow.z), true, "[ IL2CPP ]");
		render::DrawOutlinedText(gameFont, ImVec2(vars::screen_center.x, 5), 13.0f, ImColor(vars::Rainbow.x, vars::Rainbow.y, vars::Rainbow.z), true, "[ %.1f FPS ]", ImGui::GetIO().Framerate);
	}

	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(window, &mousePos);

	if (show_menu)
	{
		// X Mouse Cursor
		//render::DrawOutlinedTextForeground(gameFont, ImVec2(mousePos.x, mousePos.y), 13.0f, ImColor(vars::Rainbow.x, vars::Rainbow.y, vars::Rainbow.z), false, "X");
		DrawMenu();
	}
	// Render
	try
	{
		renderloop();
	}
	catch (...) {}

	end_scene();

	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		show_menu = !show_menu;
		ImGui::GetIO().MouseDrawCursor = show_menu;
	}

	if (GetKeyState(VK_END) & 1)
	{
		MH_DisableHook(MH_ALL_HOOKS);
		MH_Uninitialize();
		show_menu = false;
	}

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	IL2CPP::Thread::Detach(m_pThisThread);

	return oPresent(pSwapChain, SyncInterval, Flags);
}

std::string generateRandomHexString(int length) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, 15);

	std::stringstream ss;
	for (int i = 0; i < length; ++i) {
		ss << std::hex << dis(gen);
	}

	return ss.str();
}


void initchair()
{
	vars::savedHWID = generateRandomHexString(40);
#ifdef DEBUG
	CreateConsole(); // if using melonloader console is already created
	system("cls");
#endif // DEBUG
	initvars();
	find_sigs();
	EnableHooks();

	std::this_thread::sleep_for(std::chrono::milliseconds(1500));

	// IL2CPP::Callback::Initialize();

	kiero::bind(8, (void**)&oPresent, hkPresent);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Player_Cache, NULL, NULL, NULL);
	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CostumeUA, NULL, NULL, NULL);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			initchair();
			init_hook = true;
			vars::initil2cpp = true;
		}
	} while (!init_hook);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE mod, DWORD reason, LPVOID lpReserved)
{
	if (reason == 1)
	{
		DisableThreadLibraryCalls(mod);
		CreateThread(nullptr, 0, MainThread, mod, 0, nullptr);
	}
	return TRUE;
}