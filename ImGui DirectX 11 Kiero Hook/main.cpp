#include "includes.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;
HMODULE procModule;

bool alive = true;
bool show = true;

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

void Unload() {
	kiero::shutdown();
}

bool init = false;
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)& pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			ID3D11Texture2D* pBackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)& pBackBuffer);
			pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
			pBackBuffer->Release();
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
			InitImGui();
			init = true;
		}
		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();


	ImGui::NewFrame();

	ImGui::Begin("PhasHax");

	// Hacks

	// Offsets
	Address UnityPlayerbase = Memory::Internal::getModule("UnityPlayer.dll");
	Address GameAssemblybase = Memory::Internal::getModule("GameAssembly.dll");

	Address basketballScoreAddr = Memory::Internal::getAddress(UnityPlayerbase + 0x0183F0B8, { 0x8, 0x10, 0x30, 0x20, 0xD0, 0x60, 0x18 });
	Address brightnessAddr = Memory::Internal::getAddress(GameAssemblybase + 0x44AB3A, { 0x18 });

	

	// lobby header
	if (ImGui::CollapsingHeader("lobby")) {

		// basketball hack
		static int input;
		ImGui::InputInt("input the basketball value you want", &input);
		if (ImGui::Button("set")) {
			Memory::Internal::write<int>(basketballScoreAddr, input);
		}
	}
	// end of lobby

	// visuals header
	if (ImGui::CollapsingHeader("visuals")) {
		// brightness hack
		static float input;
		ImGui::SliderFloat("allows for fine tuning brightness + new limit of 10.0", &input, 0.0f, 10.0f, "brightness: %.05f");
		// Memory::Internal::write<float>(basketballScoreAddr, input);
	}
	// end of visuals


	bool exitBtn = ImGui::Button("Exit Thread.");
	if(exitBtn) {
		alive = !alive;
	}


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
	do
	{
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)& oPresent, hkPresent);
			init_hook = true;
		}
	} while (!init_hook);

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
		//MessageBox(0, "Injectected.", "Injected.", MB_OK);
		kiero::shutdown();
		break;
	}
	return TRUE;
}