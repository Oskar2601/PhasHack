#include "includes.h"

#define VERSION "v1.0"
#define MAXBYTES 100

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
HMODULE procModule;

struct Vec3 {
	float x;
	float y;
	float z;
};

typedef void (__cdecl* tSanityUpdateHook)(void* PlayerClass);

tSanityUpdateHook oSanityUpdateFunc = reinterpret_cast<tSanityUpdateHook>(0x13A3C70);
tSanityUpdateHook bSanityUpdateFunc = reinterpret_cast<tSanityUpdateHook>(0x13A3C70); // Before hook


Address basketballScoreAddr;
Address playerYPosAddr;
Address brightnessAddr;

bool alive = true;
bool show = true;
bool hooked;
bool errorOccured;

void* sanityClass = nullptr;

std::string version = VERSION;

SOCKET fp;

void hSanityUpdateFunc(void* classArg) {
	sanityClass = classArg;
	hooked = true;
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

	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

void Shutdown(std::string reason, int errCode, bool err) {
	std::ofstream myfile;
	myfile.open("Logs.txt");
	myfile << "[" << reason << "][" << errCode << "]\n";
	if (err)
		errorOccured = true;


	if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) { myfile << "\n[^^ failed to disable hooks " << "^^]\n"; }
	myfile.close();

	MH_Uninitialize();
	kiero::shutdown();
}

void InitHooks() {
	static bool hooksEnabled = false;

	
	if (MH_CreateHookApi(L"GameAssembly.dll", "Update", &hSanityUpdateFunc, reinterpret_cast<void**>(&oSanityUpdateFunc)) != MH_OK) {
		Shutdown("Failed to create api hook for SanityUpdate", 1, true);
	}
	

	if (!hooksEnabled) {
		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
			Shutdown("Failed to enable all hooks", 2, true);
		}
	}
}

void theme() {
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 4.0f;
}

void InLobby() {
	// lobby header
	if (ImGui::CollapsingHeader("lobby")) {
		static int input;

		ImGui::BulletText("Input the basketball value you want.");
		ImGui::InputInt("", &input);

		if (ImGui::Button("set")) {
			//Memory::Internal::write<int>(basketballScoreAddr, input);
		}
	}
}

void InGame() {
	ImGui::Text("Ghost room: ???");
	ImGui::Text("Ghost Type: ???");
	ImGui::Text("Ghost Age: ???");
	ImGui::Text("Ghost Room: ???");

	ImGui::Separator();

	// sanity header
	if (ImGui::CollapsingHeader("Sanity Hack")) {
		static int prevInput;
		static int input;
		static bool lockedSanity;

		ImGui::SliderInt("Changes your sanity.", &input, 0, 100, "sanity: %.0f");
		if (input != prevInput) {
			prevInput = input;
			// SanityClass.oFunc1(SanityClass.mainClass, 100 - input);
		}

		ImGui::Checkbox("Lock Sanity", &lockedSanity);
		if (lockedSanity) {
			// SanityClass.oFunc1(SanityClass.mainClass, 100 - input);
		}
	}
}

void InBoth() {
	auto windowWidth = ImGui::GetWindowSize().x;
	auto windowHeight = ImGui::GetWindowSize().y;
	auto textWidth = ImGui::CalcTextSize(version.c_str()).x;
	auto textHeight = ImGui::CalcTextSize(version.c_str()).y;

	if (ImGui::CollapsingHeader("visuals")) {
		// brightness hack
		static float input;
		ImGui::SliderFloat("allows for changing the brightness", &input, 0.0f, 10.0f, "brightness: %.05f");
	}

	if (ImGui::CollapsingHeader("movement")) {
		static float input;
		static bool locked = false;

		ImGui::SliderFloat("", &input, -100.0f, 100.0f);
		ImGui::Checkbox("Set.", &locked);

		if (locked) {
			// Memory::Internal::write<float>(playerYPosAddr, input);
		}
	}

	if (ImGui::CollapsingHeader("Debugging")) {
		if (hooked) {
			ImGui::BulletText("Function hooked and fired!!");
		}
	}

	bool exitBtn = ImGui::Button("Exit Thread.");
	if (exitBtn) {
		alive = !alive;
	}

	ImGui::SetCursorPosX(textWidth / 3);
	ImGui::SetCursorPosY(windowHeight - (textHeight * 1.5f));
	ImGui::Text(version.c_str());
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	static bool enableCursor = false;
	static bool enableGui = false;
	static bool inLobby = true;

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
			init = true;
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}
	
	/*
	try {
		inLobby = true;
		basketballScoreAddr = Memory::Internal::getAddress((uintptr_t)GetModuleHandleW(L"UnityPlayer.dll") + 0x018A8100, { 0x430, 0x310, 0x10, 0xC8, 0x58, 0x60, 0x18 });
		if (basketballScoreAddr == nullptr) {

		}
	}
	catch (const char* eName)
	{
		inLobby = false;
	}
	*/


	InitHooks();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	theme();
	ImGui::NewFrame();
	ImGui::Begin("PhasHax");

	InGame();
	InLobby();
	InBoth();

	ImGui::SameLine();
	ImGui::End();
	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	if (!alive) {
		kiero::shutdown();
		pDevice->Release();
		pContext->Release();
		pSwapChain->Release();
		oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)(oWndProc));
		oPresent(pSwapChain, SyncInterval, Flags);
		return 0;
	}

	return oPresent(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	bool init_hook = false;
	MH_STATUS __stdcall status = MH_Initialize();

	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);
	InitHooks();

	if (status != MH_OK) {
		alive = false;
	}

	while (true) {
		if (!alive) {
			alive = false;
			Sleep(1000);
			FreeLibraryAndExitThread(procModule, 0);
		}

		if (GetAsyncKeyState(VK_RSHIFT) & 1) {
			show = !show;
		}
	}

	Beep(1000, 200);


	Shutdown("Process killed", 0, false);
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hMod, 0, nullptr);
		procModule = hMod;
		break;
	case DLL_PROCESS_DETACH:
		if(errorOccured)
			Shutdown("Uninjected.", 0, false);
		break;
	}
	return TRUE;
}

/*
Error codes:

	1 - failed to init a hook
	2 - failed to enable hooks
	3 - failed to disable hooks
*/
