#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "Metadata.h"

using namespace Microsoft::WRL;

namespace Effects
{

// Heuristics of bloom changes:
// Changes to bloom consist of 2 separate parts:
// - DXHR bloom consists of 4 distinct shaders, DXHR DC - of 3
// - Constant buffers are different for draw 1 and 4
class Bloom
{
public:
	Bloom( ID3D11Device* device )
		: m_device( device )
	{
	}

	void CreateAlternatePixelShader( ID3D11PixelShader* shader, const void* bytecode, SIZE_T length );

	// Machine state functions
	ComPtr<ID3D11PixelShader> BeforePixelShaderSet( ID3D11DeviceContext* context, ID3D11PixelShader* shader );
	void BeforeDraw( ID3D11DeviceContext* context );
	void AfterDraw( ID3D11DeviceContext* context );

private:
	enum class State
	{
		Initial,
		Bloom2Set,
		Bloom2Drawn,
	};

	State m_state = State::Initial;
	ID3D11Device* m_device; // Effect cannot outlive the device

	ComPtr<ID3D11PixelShader> m_bloom3PS; // Used instead of shader2 in the second draw
	ComPtr<ID3D11Buffer> m_shader1CB; // (1.5, 0.0, 0.0, 0.0)
	ComPtr<ID3D11Buffer> m_shader4CB; // (1.5, 1.5, 1.0, 0.0)
};

};