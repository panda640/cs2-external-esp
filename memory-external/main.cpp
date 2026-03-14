#include <iostream>
#include <thread>
#include <chrono>
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <tchar.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "classes/utils.h"
#include "memory/memory.hpp"
#include "classes/vector.hpp"
#include "hacks/reader.hpp"
#include "hacks/hack.hpp"
#include "classes/globals.hpp"
#include "classes/render_dx11.hpp"
#include "classes/auto_updater.hpp"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool finish = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcA(hWnd, msg, wParam, lParam);
}

void read_thread() {
	while (!finish) {
		g_game.loop();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

int main() {
	utils.update_console_title();

	std::cout << "[info] Github Repository: https://github.com/IMXNOOBX/cs2-external-esp" << std::endl;
	std::cout << "[info] Unknowncheats thread: https://www.unknowncheats.me/forum/counter-strike-2-releases/600259-cs2-external-esp.html\n" << std::endl;

	std::cout << "[config] Reading configuration." << std::endl;
	if (config::read())
		std::cout << "[updater] Successfully read configuration file\n" << std::endl;
	else
		std::cout << "[updater] Error reading config file, resetting to the default state\n" << std::endl;

#ifndef _UC
	try {
		updater::check_and_update(config::automatic_update);
	}
	catch (std::exception& e) {
		std::cout << "An exceptio was caught while read " << e.what() << std::endl;
	}
#endif

	std::cout << "[updater] Reading offsets from file offsets.json." << std::endl;
	if (updater::read())
		std::cout << "[updater] Successfully read offsets file\n" << std::endl;
	else
		std::cout << "[updater] Error reading offsets file, resetting to the default state\n" << std::endl;

	g_game.init();

	if (g_game.buildNumber != updater::build_number) {
		std::cout << "[cs2] Build number doesnt match, the game has been updated and this esp most likely wont work." << std::endl;
		std::cout << "[warn] If the esp doesnt work, consider updating offsets manually in the file offsets.json" << std::endl;
		std::cout << "[cs2] Press any key to continue" << std::endl;
		// std::cin.get();
	}
	else {
		std::cout << "[cs2] Offsets seem to be up to date! have fun!" << std::endl;
	}

	std::cout << "[overlay] Waiting to focus game to create the overlay..." << std::endl;
	std::cout << "[overlay] Make sure your game is in \"Full Screen Windowed\"" << std::endl;
	while (GetForegroundWindow() != g_game.process->hwnd_) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		g_game.process->UpdateHWND();
		// ShowWindow(g_game.process->hwnd_, TRUE);
	}
	std::cout << "[overlay] Creating window overlay..." << std::endl;

    WNDCLASSEXA wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, "cs2-overlay", nullptr };
    ::RegisterClassExA(&wc);

    GetClientRect(g_game.process->hwnd_, &g::gameBounds);

    HWND hwnd = ::CreateWindowExA(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW, wc.lpszClassName, "cs2-external-esp", WS_POPUP, g::gameBounds.left, g::gameBounds.top, g::gameBounds.right, g::gameBounds.bottom, nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassA(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// Launch game memory reading thread
	std::thread read(read_thread);

	std::cout << "\n[settings] Press [INSERT] to toggle Menu\n" << std::endl;

	// Message loop
    bool done = false;
    while (!done && !finish)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (GetAsyncKeyState(VK_END) & 0x8000) finish = true;

        static bool insertPressed = false;
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            if (!insertPressed) {
                g::showMenu = !g::showMenu;

                LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
                if (g::showMenu) {
                    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                } else {
                    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                }
            }
            insertPressed = true;
        } else {
            insertPressed = false;
        }

        // Handle window resize
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g::showMenu)
        {
            ImGui::Begin("🎡 CS2 External ESP", &g::showMenu, ImGuiWindowFlags_AlwaysAutoResize);

            if (ImGui::BeginTabBar("Tabs")) {
                if (ImGui::BeginTabItem("Visuals")) {
                    if (ImGui::Checkbox("Box ESP", &config::show_box_esp)) config::save();
                    if (ImGui::Checkbox("Team ESP", &config::team_esp)) config::save();
                    if (ImGui::Checkbox("Skeleton ESP", &config::show_skeleton_esp)) config::save();
                    if (ImGui::Checkbox("Head Tracker", &config::show_head_tracker)) config::save();
                    if (ImGui::Checkbox("Extra Flags", &config::show_extra_flags)) config::save();

                    ImGui::Separator();
                    ImGui::Text("Render Distances");
                    if (ImGui::SliderInt("Flag Distance", &config::flag_render_distance, 0, 1000)) config::save();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Colors")) {
                    auto ColorEdit = [](const char* label, RGB& color) {
                        float col[3] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f };
                        if (ImGui::ColorEdit3(label, col)) {
                            color.r = static_cast<int>(col[0] * 255.0f);
                            color.g = static_cast<int>(col[1] * 255.0f);
                            color.b = static_cast<int>(col[2] * 255.0f);
                            return true;
                        }
                        return false;
                    };

                    if (ColorEdit("Box Enemy", config::esp_box_color_enemy)) config::save();
                    if (ColorEdit("Box Team", config::esp_box_color_team)) config::save();
                    if (ColorEdit("Skeleton Enemy", config::esp_skeleton_color_enemy)) config::save();
                    if (ColorEdit("Skeleton Team", config::esp_skeleton_color_team)) config::save();
                    if (ColorEdit("Name", config::esp_name_color)) config::save();
                    if (ColorEdit("Distance/Flags", config::esp_distance_color)) config::save();

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Misc")) {
#ifndef _UC
                    if (ImGui::Checkbox("Automatic Update", &config::automatic_update)) config::save();
#endif
                    if (ImGui::Button("Save Config")) config::save();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground);

        if (GetForegroundWindow() == g_game.process->hwnd_ || GetForegroundWindow() == hwnd) {
            hack::loop(ImGui::GetWindowDrawList());
        }

        ImGui::End();

        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassA(wc.lpszClassName, wc.hInstance);

	read.detach();

	g_game.close();

	return 1;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}
