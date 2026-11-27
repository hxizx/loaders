// Absent Loader — Win32 + DirectX 11 + Dear ImGui
// Licensed via KeyAuth (C++ client matching the C# api init credentials).

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "keyauth/auth.hpp"

#include <d3d11.h>
#include <tchar.h>
#include <windows.h>
#include <string>
#include <cstring>
#include <exception>

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void ApplyAbsentStyle();
static void DrawLoginWindow();
static void DrawDashboardWindow();

// ---------------------------------------------------------------------------
// KeyAuth — C++ equivalent of the C# dashboard snippet:
//
//   public static api KeyAuthApp = new api(
//       name: "evolution",
//       ownerid: "UISzwEIYml",
//       version: "1.0"
//       //path: @"Your_Path_Here"
//   );
// ---------------------------------------------------------------------------
static KeyAuth::api KeyAuthApp(
    "evolution",
    "UISzwEIYml",
    "1.0"
    // , "Your_Path_Here"
);

static bool g_keyauth_ready = false;
static bool g_licensed = false;
static char g_license_key[128] = "";
static std::string g_status_message = "Connecting to license server...";
static bool g_status_error = false;
static bool g_login_busy = false;

static void ApplyAbsentStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Spacing / shape — keep rounded product look, tighten consistency
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;

    style.WindowPadding = ImVec2(20.0f, 18.0f);
    style.FramePadding = ImVec2(12.0f, 8.0f);
    style.ItemSpacing = ImVec2(10.0f, 10.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 16.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    // Absent Loader palette — deep charcoal + violet accent (existing brand)
    ImVec4* colors = style.Colors;
    const ImVec4 bg         = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
    const ImVec4 bg_soft    = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    const ImVec4 frame      = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
    const ImVec4 frame_hov  = ImVec4(0.18f, 0.16f, 0.24f, 1.00f);
    const ImVec4 accent     = ImVec4(0.65f, 0.28f, 0.98f, 1.00f);
    const ImVec4 accent_hov = ImVec4(0.72f, 0.40f, 1.00f, 1.00f);
    const ImVec4 accent_act = ImVec4(0.50f, 0.18f, 0.80f, 1.00f);
    const ImVec4 text       = ImVec4(0.94f, 0.94f, 0.97f, 1.00f);
    const ImVec4 text_dim   = ImVec4(0.55f, 0.55f, 0.62f, 1.00f);
    const ImVec4 border     = ImVec4(0.22f, 0.20f, 0.30f, 1.00f);

    colors[ImGuiCol_Text]                 = text;
    colors[ImGuiCol_TextDisabled]         = text_dim;
    colors[ImGuiCol_WindowBg]             = bg;
    colors[ImGuiCol_ChildBg]              = bg_soft;
    colors[ImGuiCol_PopupBg]              = bg_soft;
    colors[ImGuiCol_Border]               = border;
    colors[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_FrameBg]              = frame;
    colors[ImGuiCol_FrameBgHovered]       = frame_hov;
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.22f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]              = bg;
    colors[ImGuiCol_TitleBgActive]        = bg_soft;
    colors[ImGuiCol_TitleBgCollapsed]     = bg;
    colors[ImGuiCol_MenuBarBg]            = bg_soft;
    colors[ImGuiCol_ScrollbarBg]          = bg;
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.28f, 0.26f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.32f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = accent;
    colors[ImGuiCol_CheckMark]            = accent;
    colors[ImGuiCol_SliderGrab]           = accent;
    colors[ImGuiCol_SliderGrabActive]     = accent_act;
    colors[ImGuiCol_Button]               = accent;
    colors[ImGuiCol_ButtonHovered]        = accent_hov;
    colors[ImGuiCol_ButtonActive]         = accent_act;
    colors[ImGuiCol_Header]               = ImVec4(0.28f, 0.18f, 0.42f, 0.70f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.40f, 0.24f, 0.58f, 0.85f);
    colors[ImGuiCol_HeaderActive]         = accent_act;
    colors[ImGuiCol_Separator]            = border;
    colors[ImGuiCol_SeparatorHovered]     = accent_hov;
    colors[ImGuiCol_SeparatorActive]      = accent;
    colors[ImGuiCol_ResizeGrip]           = ImVec4(accent.x, accent.y, accent.z, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]    = ImVec4(accent.x, accent.y, accent.z, 0.55f);
    colors[ImGuiCol_ResizeGripActive]     = accent;
    colors[ImGuiCol_Tab]                  = bg_soft;
    colors[ImGuiCol_TabHovered]           = accent_hov;
    colors[ImGuiCol_TabActive]            = ImVec4(0.32f, 0.20f, 0.48f, 1.00f);
    colors[ImGuiCol_PlotLines]            = accent;
    colors[ImGuiCol_PlotHistogram]        = accent;
    colors[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_NavHighlight]         = accent;
}

static void CenterText(const char* text)
{
    const float window_width = ImGui::GetWindowSize().x;
    const float text_width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((window_width - text_width) * 0.5f);
    ImGui::TextUnformatted(text);
}

static void DrawLoginWindow()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420.0f, 260.0f), ImGuiCond_Always);

    ImGui::Begin("Absent Loader", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.40f, 1.00f, 1.00f));
        CenterText("ABSENT");
        ImGui::PopStyleColor();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.70f, 0.78f, 1.00f));
        CenterText("LOADER");
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    ImGui::TextDisabled("License key");
    ImGui::SetNextItemWidth(-1.0f);
    const bool enter_pressed = ImGui::InputText(
        "##license_key",
        g_license_key,
        IM_ARRAYSIZE(g_license_key),
        ImGuiInputTextFlags_Password |
        ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::Dummy(ImVec2(0.0f, 4.0f));

    const bool can_login = g_keyauth_ready && !g_login_busy && g_license_key[0] != '\0';
    if (!can_login)
        ImGui::BeginDisabled();

    if (ImGui::Button(g_login_busy ? "Signing in..." : "Login", ImVec2(-1.0f, 36.0f)) || (enter_pressed && can_login))
    {
        g_login_busy = true;
        g_status_error = false;
        g_status_message = "Validating license...";

        try
        {
            KeyAuthApp.license(g_license_key);
            if (KeyAuthApp.response.success)
            {
                g_licensed = true;
                g_status_error = false;
                g_status_message = KeyAuthApp.response.message.empty()
                    ? "License accepted"
                    : KeyAuthApp.response.message;
                // Clear key from memory after successful auth
                SecureZeroMemory(g_license_key, sizeof(g_license_key));
            }
            else
            {
                g_status_error = true;
                g_status_message = KeyAuthApp.response.message.empty()
                    ? "License rejected"
                    : KeyAuthApp.response.message;
            }
        }
        catch (const std::exception& ex)
        {
            g_status_error = true;
            g_status_message = ex.what();
        }
        g_login_busy = false;
    }

    if (!can_login)
        ImGui::EndDisabled();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    {
        const ImVec4 status_col = g_status_error
            ? ImVec4(0.95f, 0.35f, 0.40f, 1.00f)
            : ImVec4(0.55f, 0.55f, 0.62f, 1.00f);
        ImGui::PushStyleColor(ImGuiCol_Text, status_col);
        ImGui::TextWrapped("%s", g_status_message.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

static void DrawDashboardWindow()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420.0f, 220.0f), ImGuiCond_Always);

    ImGui::Begin("Absent Loader##dashboard", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.40f, 1.00f, 1.00f));
    CenterText("ABSENT");
    ImGui::PopStyleColor();
    CenterText("Welcome back");

    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.0f, 10.0f));

    ImGui::TextDisabled("Status");
    ImGui::TextWrapped("%s", g_status_message.c_str());

    if (!KeyAuthApp.user_data.username.empty())
    {
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        ImGui::TextDisabled("User");
        ImGui::TextUnformatted(KeyAuthApp.user_data.username.c_str());
    }

    if (!KeyAuthApp.user_data.expires.empty())
    {
        ImGui::TextDisabled("Expiry");
        ImGui::TextUnformatted(KeyAuthApp.user_data.expires.c_str());
    }

    ImGui::Dummy(ImVec2(0.0f, 12.0f));
    if (ImGui::Button("Sign out", ImVec2(-1.0f, 32.0f)))
    {
        g_licensed = false;
        g_status_error = false;
        g_status_message = "Signed out. Enter a license key to continue.";
        SecureZeroMemory(g_license_key, sizeof(g_license_key));
    }

    ImGui::End();
}

// Main code
int main(int, char**)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Absent Loader", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Absent Loader", WS_OVERLAPPEDWINDOW, 100, 100, (int)(900 * main_scale), (int)(600 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr; // don't persist layout next to the exe

    ApplyAbsentStyle();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Prefer Segoe UI when present (already the common Windows UI font path in the stock example).
    ImFont* segoe = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f * main_scale);
    if (segoe == nullptr)
        io.Fonts->AddFontDefault();

    // Clear / backdrop color — slightly cooler than the stock demo gray
    ImVec4 clear_color = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);

    // Initialize KeyAuth before the interactive login loop
    try
    {
        KeyAuthApp.init();
        if (KeyAuthApp.response.success)
        {
            g_keyauth_ready = true;
            g_status_error = false;
            g_status_message = "Enter your license key to continue.";
        }
        else
        {
            g_keyauth_ready = false;
            g_status_error = true;
            g_status_message = KeyAuthApp.response.message.empty()
                ? "License service unavailable"
                : KeyAuthApp.response.message;
        }
    }
    catch (const std::exception& ex)
    {
        g_keyauth_ready = false;
        g_status_error = true;
        g_status_message = ex.what();
    }

    bool done = false;
    while (!done)
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

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

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

        if (g_licensed)
            DrawDashboardWindow();
        else
            DrawLoginWindow();

        ImGui::Render();

        const float clear_color_with_alpha[4] = {
            clear_color.x * clear_color.w,
            clear_color.y * clear_color.w,
            clear_color.z * clear_color.w,
            clear_color.w
        };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
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
    if (res == DXGI_ERROR_UNSUPPORTED)
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

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
