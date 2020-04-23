#pragma once

#include <Unknwn.h>

#include <utility>


// Tiny extension interface for D3D11/DXGI wrappers so they can be "aware"
// of each other's extensions and query for the real underlying interface

__interface __declspec(uuid("5F7408E6-77F3-4668-B1F0-9969E803BD9A")) IWrapperObject : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE GetUnderlyingInterface(REFIID riid, void** ppvObject);
};


// Convenience wrapper for wil::unique_hmodule, so we let it leak if the interface it wraps is still referenced somewhere
class SafeUniqueHmodule final
{
public:
	SafeUniqueHmodule( wil::unique_hmodule module, Microsoft::WRL::ComPtr<IUnknown> instance )
		: m_module( std::move(module) ), m_instance( std::move(instance) )
	{
	}

	~SafeUniqueHmodule()
	{
		ULONG ref = m_instance.Reset();
		if ( ref != 0 )
		{
			// Let the module leak
			m_module.release();
		}
	}

private:
	wil::unique_hmodule m_module;
	Microsoft::WRL::ComPtr<IUnknown> m_instance;
};