#include "WrappedDXGI.h"

#include <utility>
#include <d3d11.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include "effects/Metadata.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Functions specific to ImGui
namespace UI
{

static bool imguiInitialized = false;
static WNDPROC orgWndProc;
LRESULT WINAPI UIWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT imguiResult = ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    if ( imguiResult != 0 ) return imguiResult;

    const ImGuiIO& io = ImGui::GetIO();
    const bool captureMouse = io.WantCaptureMouse || io.MouseDrawCursor;
    const bool captureKeyboard = io.WantCaptureKeyboard;
    if ( captureMouse || captureKeyboard )
    {
        if ( captureMouse && (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) ) return imguiResult;
        if ( captureKeyboard && (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ) return imguiResult;

        // Filter Raw Input
        if ( msg == WM_INPUT )
        {
            RAWINPUTHEADER header;
            UINT size = sizeof(header);

            if ( GetRawInputData( reinterpret_cast<HRAWINPUT >(lParam), RID_HEADER, &header, &size, sizeof(RAWINPUTHEADER) ) != -1 )
            {
                if ( (captureMouse && header.dwType == RIM_TYPEMOUSE) || (captureKeyboard && header.dwType == RIM_TYPEKEYBOARD) )
                {
                    // Let the OS perform cleanup
                    return DefWindowProc(hWnd, msg, wParam, lParam);
                }
            }
        }
    }

    return CallWindowProc(orgWndProc, hWnd, msg, wParam, lParam);
}

static void SetUpImGuiStyle()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.06f, 0.06f, 0.06f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.81f, 0.83f, 0.81f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.93f, 0.65f, 0.14f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style->FramePadding = ImVec2(4, 2);
    style->ItemSpacing = ImVec2(10, 2);
    style->IndentSpacing = 12;
    style->ScrollbarSize = 10;

    style->WindowRounding = 4;
    style->FrameRounding = 4;
    style->PopupRounding = 4;
    style->ScrollbarRounding = 6;
    style->GrabRounding = 4;
    style->TabRounding = 4;

    style->WindowTitleAlign = ImVec2(1.0f, 0.5f);
    style->WindowMenuButtonPosition = ImGuiDir_Right;

    style->DisplaySafeAreaPadding = ImVec2(4, 4);
}

}

extern HMODULE WINAPI LoadLibraryA_DXHR( LPCSTR lpLibFileName );

HRESULT WINAPI CreateDXGIFactory_Export( REFIID riid, void** ppFactory )
{
    *ppFactory = nullptr;

    // Try to load a real dxgi.dll, DXGIFactory will claim ownership of its reference if created successfully
    wil::unique_hmodule dxgiModule( LoadLibraryA_DXHR("dxgi") );
    if ( dxgiModule.is_valid() )
    {
        auto factoryFn = reinterpret_cast<decltype(CreateDXGIFactory)*>(GetProcAddress( dxgiModule.get(), "CreateDXGIFactory" ));
        if ( factoryFn != nullptr )
        {
            ComPtr<IDXGIFactory> factory;
            HRESULT hr = factoryFn( IID_PPV_ARGS(factory.GetAddressOf()) );
            if ( SUCCEEDED(hr) )
            {
                ComPtr<IDXGIFactory> wrappedFactory = Make<DXGIFactory>( std::move(dxgiModule), std::move(factory) );
                wrappedFactory.CopyTo( riid, ppFactory );
            }
            return hr;
        }
    }

	return DXGI_ERROR_SDK_COMPONENT_MISSING; // If either LoadLibrary or GetProcAddress fails
}

// ====================================================

DXGIFactory::DXGIFactory(wil::unique_hmodule module, ComPtr<IDXGIFactory> factory)
    : m_dxgiModule( std::move(module), factory ), m_orig( std::move(factory) )
{
}

HRESULT STDMETHODCALLTYPE DXGIFactory::QueryInterface(REFIID riid, void** ppvObject)
{
    HRESULT hr = __super::QueryInterface(riid, ppvObject);
    if ( FAILED(hr) )
    {
        hr = m_orig->QueryInterface(riid, ppvObject);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE DXGIFactory::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return m_orig->SetPrivateData(Name, DataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return m_orig->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return m_orig->GetPrivateData(Name, pDataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::GetParent(REFIID riid, void** ppParent)
{
    return m_orig->GetParent(riid, ppParent);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter)
{
    return m_orig->EnumAdapters(Adapter, ppAdapter);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
    return m_orig->MakeWindowAssociation(WindowHandle, Flags);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::GetWindowAssociation(HWND* pWindowHandle)
{
    return m_orig->GetWindowAssociation(pWindowHandle);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    if ( pDevice == nullptr || pDesc == nullptr || ppSwapChain == nullptr ) return DXGI_ERROR_INVALID_CALL;
    *ppSwapChain = nullptr;

    ComPtr<IDXGISwapChain> swapChain;
    HRESULT hr = S_OK;
    bool created = false;

    ComPtr<IWrapperObject> wrapper;
    if ( SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(wrapper.GetAddressOf()))) )
    {
        ComPtr<IUnknown> underlyingInterface;
        if ( SUCCEEDED(wrapper->GetUnderlyingInterface(IID_PPV_ARGS(underlyingInterface.GetAddressOf()))) )
        {
            hr = m_orig->CreateSwapChain(underlyingInterface.Get(), pDesc, swapChain.GetAddressOf());
            created = true;
        }
    }

    if ( !created )
    {
        hr = m_orig->CreateSwapChain(pDevice, pDesc, swapChain.GetAddressOf());
    }

    if ( SUCCEEDED(hr) )
    {
        ComPtr<IDXGISwapChain> wrappedSwapChain = Make<DXGISwapChain>( std::move(swapChain), this, pDevice, pDesc );
        *ppSwapChain = wrappedSwapChain.Detach();
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE DXGIFactory::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
    return m_orig->CreateSoftwareAdapter(Module, ppAdapter);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::GetUnderlyingInterface(REFIID riid, void** ppvObject)
{
    return m_orig.CopyTo(riid, ppvObject);
}

// ====================================================

DXGISwapChain::DXGISwapChain(ComPtr<IDXGISwapChain> swapChain, ComPtr<DXGIFactory> factory, ComPtr<IUnknown> device, const DXGI_SWAP_CHAIN_DESC* desc)
    : m_factory( std::move(factory) ), m_device( std::move(device) ), m_orig( std::move(swapChain) )
{
    // We set up Dear Imgui in swapchain constructor, don't tear it down as it's pointless
    if ( !std::exchange(UI::imguiInitialized, true) )
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        // Setup Dear ImGui style
        UI::SetUpImGuiStyle();

        // Hook into the window proc
        UI::orgWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr( desc->OutputWindow, GWLP_WNDPROC ));
        SetWindowLongPtr( desc->OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(UI::UIWndProc) );
    }

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(desc->OutputWindow);

    ComPtr<ID3D11Device> d3dDevice;
    if ( SUCCEEDED(m_device.As(&d3dDevice)) )
    {
        ComPtr<ID3D11DeviceContext> d3dDeviceContext;
        d3dDevice->GetImmediateContext( d3dDeviceContext.GetAddressOf() );

        ImGui_ImplDX11_Init(d3dDevice.Get(), d3dDeviceContext.Get()); // Init holds a reference to both
    }

    // Immediately start a new frame - we'll be starting new frames after each Present
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

DXGISwapChain::~DXGISwapChain()
{
    ImGui::EndFrame();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::QueryInterface(REFIID riid, void** ppvObject)
{
    HRESULT hr = __super::QueryInterface(riid, ppvObject);
    if ( FAILED(hr) )
    {
        hr = m_orig->QueryInterface(riid, ppvObject);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
	return m_orig->SetPrivateData(Name, DataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
	return m_orig->SetPrivateDataInterface(Name, pUnknown);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
	return m_orig->GetPrivateData(Name, pDataSize, pData);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetParent(REFIID riid, void** ppParent)
{
	return m_factory.CopyTo(riid, ppParent);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetDevice(REFIID riid, void** ppDevice)
{
	return m_device.CopyTo(riid, ppDevice);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
    // Draw all UI widgets
    {
        using namespace Effects;

        ImGuiIO& io = ImGui::GetIO();

        static bool keyPressed = false;
        if ( io.KeysDown[VK_F11] )
        {
            if ( !keyPressed )
            {
                SETTINGS.isShown = !SETTINGS.isShown;
                keyPressed = true;
            }
        }
        else
        {
            keyPressed = false;
        }

        if ( SETTINGS.isShown )
        {
            //ImGui::ShowDemoWindow();
            constexpr float DIST_FROM_CORNER = 20.0f;
            const ImVec2 window_pos = ImVec2(io.DisplaySize.x - DIST_FROM_CORNER, DIST_FROM_CORNER);
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once, ImVec2(1.0f, 0.0f));
            if ( ImGui::Begin( "DXHRDC-GFX Settings", &SETTINGS.isShown, ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_AlwaysVerticalScrollbar ) )
            {
                bool needsToSave = false;

                int id = 0;

                needsToSave |= ImGui::Checkbox("Enable Gold Filter", &SETTINGS.colorGradingEnabled);

                ImGui::PushID(id++);
                ImGui::Text( "Bloom Style" );
                needsToSave |= ImGui::RadioButton("DX:HR", &SETTINGS.bloomType, 1); ImGui::SameLine();
                needsToSave |= ImGui::RadioButton("DX:HR DC", &SETTINGS.bloomType, 0);
                ImGui::PopID();

                ImGui::PushID(id++);
                ImGui::Text( "Lighting Style" );
                needsToSave |= ImGui::RadioButton("DX:HR", &SETTINGS.lightingType, 2);
                needsToSave |= ImGui::RadioButton("DX:HR DC (Fixed)", &SETTINGS.lightingType, 1); ImGui::SameLine();
                needsToSave |= ImGui::RadioButton("DX:HR DC", &SETTINGS.lightingType, 0);
                ImGui::PopID();

                ImGui::Separator();

                if ( SETTINGS.colorGradingEnabled )
                {
                    bool colorGradingDirty = false;

                    ImGui::Text( "Gold Filter Settings:" );

                    // Presets
                    const int curPreset = GetSelectedPreset( SETTINGS.colorGradingAttributes );

                    const int numPresets = _countof( COLOR_GRADING_PRESETS );
                    for ( int i = 0; i < numPresets; i++ )
                    {
                        char buf[16];
                        sprintf_s( buf, "Preset %u", i + 1 );
                        if ( ImGui::RadioButton( buf, curPreset == i ) )
                        {
                            memcpy( &SETTINGS.colorGradingAttributes[0], COLOR_GRADING_PRESETS[i], sizeof(COLOR_GRADING_PRESETS[i]) );
                            colorGradingDirty = true;                      
                        }
                        ImGui::SameLine();
                    }
                    ImGui::NewLine();

                    if ( ImGui::CollapsingHeader( "Advanced settings" ) )
                    {
                        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.45f);
                        colorGradingDirty |= ImGui::DragFloat( "Intensity", &SETTINGS.colorGradingAttributes[0][0], 0.005f, 0.0f, FLT_MAX );
                        colorGradingDirty |= ImGui::DragFloat( "Saturation", &SETTINGS.colorGradingAttributes[0][1], 0.005f, 0.0f, FLT_MAX );
                        colorGradingDirty |= ImGui::DragFloat( "Temp. threshold", &SETTINGS.colorGradingAttributes[0][2], 0.005f, 0.0f, FLT_MAX );
                        ImGui::PopItemWidth();
                

                        ImGui::NewLine();
                        colorGradingDirty |= ImGui::ColorEdit3( "Cold", SETTINGS.colorGradingAttributes[1] );
                        colorGradingDirty |= ImGui::ColorEdit3( "Moderate", SETTINGS.colorGradingAttributes[2] );
                        colorGradingDirty |= ImGui::ColorEdit3( "Warm", SETTINGS.colorGradingAttributes[3] );
                        if ( ImGui::Button( "Restore defaults##Colors" ) )
                        {
                            memcpy( &SETTINGS.colorGradingAttributes[0], COLOR_GRADING_PRESETS[0], sizeof(COLOR_GRADING_PRESETS[0]) );
                            colorGradingDirty = true;
                        }

                        ImGui::NewLine();
                        colorGradingDirty |= ImGui::DragFloat4( "Vignette", SETTINGS.colorGradingAttributes[4], 0.05f, 0.0f, FLT_MAX, "%.2f" );
                        if ( ImGui::Button( "Restore defaults##Vignette" ) )
                        {
                            memcpy( &SETTINGS.colorGradingAttributes[4], Effects::VIGNETTE_PRESET, sizeof(Effects::VIGNETTE_PRESET) );
                            colorGradingDirty = true;
                        }
                    }

                    SETTINGS.colorGradingDirty |= colorGradingDirty;
                    needsToSave |= colorGradingDirty;

                    ImGui::Dummy( ImVec2(0.0f, 20.0f) );
                }

                if ( needsToSave )
                {
                    SaveSettings();
                }
            }

            ImGui::End();
        }

        io.MouseDrawCursor = SETTINGS.isShown;
    }

    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();
    if ( drawData->TotalVtxCount > 0 )
    {
        // Only do this relatively heavy work if we actually render something
        ComPtr<ID3D11Device> d3dDevice;
        if ( SUCCEEDED(m_device.As(&d3dDevice)) )
        {
            ComPtr<ID3D11Resource> renderTarget;
            if ( SUCCEEDED(GetBuffer(0, IID_PPV_ARGS(renderTarget.GetAddressOf())))  )
            {
                ComPtr<ID3D11RenderTargetView> rtv;
                if ( SUCCEEDED(d3dDevice->CreateRenderTargetView( renderTarget.Get(), nullptr, rtv.GetAddressOf()) ) )
                {
                    ComPtr<ID3D11DeviceContext> d3dDeviceContext;
                    d3dDevice->GetImmediateContext( d3dDeviceContext.GetAddressOf() );

                    d3dDeviceContext->OMSetRenderTargets( 1, rtv.GetAddressOf(), nullptr );
                }
            }
        }
    }

    ImGui_ImplDX11_RenderDrawData(drawData);

    HRESULT hr = m_orig->Present(SyncInterval, Flags);

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    return hr;
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
	return m_orig->GetBuffer(Buffer, riid, ppSurface);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
	return m_orig->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
	return m_orig->GetFullscreenState(pFullscreen, ppTarget);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
	return m_orig->GetDesc(pDesc);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
	return m_orig->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
	return m_orig->ResizeTarget(pNewTargetParameters);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetContainingOutput(IDXGIOutput** ppOutput)
{
	return m_orig->GetContainingOutput(ppOutput);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
	return m_orig->GetFrameStatistics(pStats);
}

HRESULT STDMETHODCALLTYPE DXGISwapChain::GetLastPresentCount(UINT* pLastPresentCount)
{
	return m_orig->GetLastPresentCount(pLastPresentCount);
}
