#pragma once

#include <Unknwn.h>


// Tiny extension interface for D3D11/DXGI wrappers so they can be "aware"
// of each other's extensions and query for the real underlying interface

__interface __declspec(uuid("5F7408E6-77F3-4668-B1F0-9969E803BD9A")) IWrapperObject : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetUnderlyingInterface(REFIID riid, void** ppvObject);
};