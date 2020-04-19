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

static bool hasImGuiContext = false;
static WNDPROC orgWndProc;
LRESULT WINAPI UIWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if ( hasImGuiContext )
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
    }

    return CallWindowProc(orgWndProc, hWnd, msg, wParam, lParam);
}

}

HRESULT WINAPI CreateDXGIFactory_Export( REFIID riid, void** ppFactory )
{
    *ppFactory = nullptr;

    // Try to load a real dxgi.dll, DXGIFactory will claim ownership of its reference if created successfully
    wil::unique_hmodule dxgiModule( LoadLibrary(L"dxgi") );
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
    : m_dxgiModule( std::move(module) ), m_orig( std::move(factory) )
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
    // We set up Dear Imgui in swapchain constructor and tear it down in the destructor
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    UI::hasImGuiContext = true;

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(desc->OutputWindow);

    ComPtr<ID3D11Device> d3dDevice;
    if ( SUCCEEDED(m_device.As(&d3dDevice)) )
    {
        ComPtr<ID3D11DeviceContext> d3dDeviceContext;
        d3dDevice->GetImmediateContext( d3dDeviceContext.GetAddressOf() );

        ImGui_ImplDX11_Init(d3dDevice.Get(), d3dDeviceContext.Get()); // Init holds a reference to both
    }

    // Hook into the window proc (only once per session)
    if ( UI::orgWndProc == nullptr )
    {
        UI::orgWndProc = reinterpret_cast<WNDPROC>(GetWindowLongPtr( desc->OutputWindow, GWLP_WNDPROC ));
        SetWindowLongPtr( desc->OutputWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(UI::UIWndProc) );
    }

    // Immediately start a new frame - we'll be starting new frames after each Present
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

DXGISwapChain::~DXGISwapChain()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    UI::hasImGuiContext = false;
    ImGui::DestroyContext();
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
            if ( ImGui::Begin( "DXHRDC-GFX Settings", &SETTINGS.isShown, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse ) )
            {
                int id = 0;

                ImGui::Checkbox("Enable Gold Filter", &SETTINGS.colorGradingEnabled);

                ImGui::PushID(id++);
                ImGui::Text( "Bloom Style" );
                ImGui::RadioButton("DX:HR", &SETTINGS.bloomType, 1); ImGui::SameLine();
                ImGui::RadioButton("DX:HR DC", &SETTINGS.bloomType, 0);
                ImGui::PopID();

                ImGui::PushID(id++);
                ImGui::Text( "Lighting Style" );
                ImGui::RadioButton("DX:HR", &SETTINGS.lightingType, 2);
                ImGui::RadioButton("DX:HR DC (Fixed)", &SETTINGS.lightingType, 1); ImGui::SameLine();
                ImGui::RadioButton("DX:HR DC", &SETTINGS.lightingType, 0);
                ImGui::PopID();
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
