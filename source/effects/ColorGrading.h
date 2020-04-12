#pragma once

#include <d3d11.h>
#include <type_traits>

namespace Effects
{

// Private data we attach to "interesting" resources
struct __declspec(uuid("5383C3EB-9DE8-48FC-8C88-8721759EA8E6")) ResourceMetadata
{
	// Found by hashing
	enum class Type
	{
		BloomMergerShader,
	};

	Type m_type;
};
static_assert(std::is_trivial_v<ResourceMetadata>); // Private data is memcpy'd around and destructed by freeing memory only


// Heuristics of color grading:
// 1. Find the bloom merger call
// 2. From this draw call, save the following - vertex shader, input layout, rasterizer state, blend state (DS state seems to be same)
// 2. Skip until the first DrawIndexed call after the bloom merger call - this will skip over AA (if any)
// 3. Output RT of this DrawIndexed call is the output we need to apply color grading on
// TODO: CopyResource can be skipped if AA is performed - need to cache input of the bloom merger call and re-route the next non-indexed Draw call,
//	     then apply color grading
class ColorGrading
{
public:
	ColorGrading( ID3D11Device* device )
		: m_device( device )
	{
	}

	void AnnotatePixelShader( ID3D11PixelShader* shader, const void* bytecode, SIZE_T length );

	// Machine state functions
	void OnPixelShaderSet( ID3D11PixelShader* shader );
	void BeforeDraw( ID3D11DeviceContext* context );
	void BeforeDrawIndexed( ID3D11DeviceContext* context );
	void BeforeSetIndexBuffer( ID3D11DeviceContext* context );

private:
	enum class State
	{
		Initial,
		MergerCallFound,
		ResourcesGathered,
	};

	State m_state = State::Initial;
	ID3D11Device* m_device; // ColorGrading class cannot outlive the device
};

};