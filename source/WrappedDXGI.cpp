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
    if ( pDevice == nullptr ) return DXGI_ERROR_INVALID_CALL;

    ComPtr<IWrapperObject> wrapper;
    if ( SUCCEEDED(pDevice->QueryInterface(IID_PPV_ARGS(wrapper.GetAddressOf()))) )
    {
        ComPtr<IUnknown> underlyingInterface;
        if ( SUCCEEDED(wrapper->GetUnderlyingInterface(IID_PPV_ARGS(underlyingInterface.GetAddressOf()))) )
        {
            return m_orig->CreateSwapChain(underlyingInterface.Get(), pDesc, ppSwapChain);
        }
    }

    return m_orig->CreateSwapChain(pDevice, pDesc, ppSwapChain);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
    return m_orig->CreateSoftwareAdapter(Module, ppAdapter);
}

HRESULT STDMETHODCALLTYPE DXGIFactory::GetUnderlyingInterface(REFIID riid, void** ppvObject)
{
    return m_orig.CopyTo(riid, ppvObject);
}
