#pragma once

#include <dxgi.h>

#include <wrl/implements.h>
#include <wrl/client.h>
#include "wil/resource.h"

using namespace Microsoft::WRL;


// Wrapped DXGI interfaces which are mostly passthrough, but they are also
// aware of our D3D11 wrappers

class DXGIFactory final : public RuntimeClass< RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDXGIFactory, IDXGIObject> >
{
public:
	DXGIFactory( wil::unique_hmodule module, ComPtr<IDXGIFactory> factory );

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override; // Overload to allow DXGI to query for internal, undocumented interfaces

	// IDXGIFactory
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override;
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;
	virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter) override;
	virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags) override;
	virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* pWindowHandle) override;
	virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain) override;
	virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter) override;

private:
	wil::unique_hmodule m_dxgiModule;
    ComPtr<IDXGIFactory> m_orig;
};