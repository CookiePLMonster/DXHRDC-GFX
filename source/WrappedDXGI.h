#pragma once

#include <dxgi.h>

#include <wrl/implements.h>
#include <wrl/client.h>
#include "wil/resource.h"

#include "WrappedExtension.h"

using namespace Microsoft::WRL;


// Wrapped DXGI interfaces which are mostly passthrough, but they are also
// aware of our D3D11 wrappers

class DXGIFactory final : public RuntimeClass< RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDXGIFactory, IDXGIObject>, IWrapperObject >
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

	// IWrapperObject
	virtual HRESULT STDMETHODCALLTYPE GetUnderlyingInterface(REFIID riid, void** ppvObject) override;

private:
	wil::unique_hmodule m_dxgiModule;
    ComPtr<IDXGIFactory> m_orig;
};

class DXGISwapChain final : public RuntimeClass< RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDXGISwapChain, IDXGIDeviceSubObject, IDXGIObject> >
{
public:
	DXGISwapChain(ComPtr<IDXGISwapChain> swapChain, ComPtr<DXGIFactory> factory, ComPtr<IUnknown> device);

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override; // Overload to allow DXGI to query for internal, undocumented interfaces

	// IDXGISwapChain
	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override;
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override;
	virtual HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override;
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) override;
	virtual HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override;
	virtual HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) override;
	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) override;
	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) override;
	virtual HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) override;
	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override;
	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) override;
	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput) override;
	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override;
	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount) override;

private:
	ComPtr<DXGIFactory> m_factory;
	ComPtr<IUnknown> m_device;
	ComPtr<IDXGISwapChain> m_orig;
};