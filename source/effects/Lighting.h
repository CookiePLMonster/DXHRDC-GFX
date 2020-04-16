#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Metadata.h"

using namespace Microsoft::WRL;

namespace Effects
{

// Heuristics of bloom changes:
// For LightShader1 and LightShader2, just create alternate resources and swap out pixel shaders accordingly
// For LightShader3, swap out pixel shader and swap SRV0 with SRV1
class Lighting
{
public:
	Lighting( ID3D11Device* device )
		: m_device( device )
	{
	}

	void CreateAlternatePixelShader( ID3D11PixelShader* shader, const void* bytecode, SIZE_T length );

	bool OnDrawIndexed( ID3D11DeviceContext* context, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation );
	ComPtr<ID3D11PixelShader> BeforePixelShaderSet( ID3D11DeviceContext* context, ID3D11PixelShader* shader );

private:
	ID3D11Device* m_device; // Effect cannot outlive the device
	bool m_swapSRVs = false; // For LightingShader3
};

}