#include "WrappedDXGI.h"

#include <utility>


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
    if ( pDevice == nullptr || ppSwapChain == nullptr ) return DXGI_ERROR_INVALID_CALL;
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
        ComPtr<IDXGISwapChain> wrappedSwapChain = Make<DXGISwapChain>( std::move(swapChain), this, pDevice );
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

DXGISwapChain::DXGISwapChain(ComPtr<IDXGISwapChain> swapChain, ComPtr<DXGIFactory> factory, ComPtr<IUnknown> device)
    : m_factory( std::move(factory) ), m_device( std::move(device) ), m_orig( std::move(swapChain) )
{
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
	return m_orig->Present(SyncInterval, Flags);
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
