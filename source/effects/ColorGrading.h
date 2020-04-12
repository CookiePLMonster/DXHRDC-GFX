#pragma once

#include <d3d11.h>

#include <type_traits>
#include <optional>
#include <tuple>

#include <wrl/client.h>

using namespace Microsoft::WRL;

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
// 3. Skip until the first blend state change - entire postprocessing uses the same blend state, subtitles/UI do not
// 4. Output RT of the draw call to follow is the output we need to apply color grading on
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
	void BeforeDraw( ID3D11DeviceContext* context, UINT VertexCount, UINT StartVertexLocation );
	void BeforeOMSetBlendState( ID3D11DeviceContext* context, ID3D11BlendState* pBlendState );

private:
	void CreatePersistentData();

	enum class State
	{
		Initial,
		MergerCallFound,
		ResourcesGathered,
	};

	State m_state = State::Initial;
	ID3D11Device* m_device; // ColorGrading class cannot outlive the device

	// Persistent data - created on demand and invalidated only on resolution/settings change
	struct PersistentData
	{
		ComPtr<ID3D11PixelShader> m_pixelShader;
		ComPtr<ID3D11Buffer> m_constantBuffer;

		// Flushed if RTV doesn't match the current output
		ComPtr<ID3D11Resource> m_lastOutputRT;
		ComPtr<ID3D11ShaderResourceView> m_lastOutputSRV;
		
		// Flushed if RT dimensions don't match the current output
		std::tuple< ComPtr<ID3D11Texture2D>, UINT, UINT > m_tempRT; // RT, Width, Height
		ComPtr<ID3D11RenderTargetView> m_tempRTV;
	};

	// Volatile data - references obtained and released every frame, used for draw detection
	struct VolatileData
	{
		ComPtr<ID3D11VertexShader> m_vertexShader;
		ComPtr<ID3D11InputLayout> m_inputLayout;
		ComPtr<ID3D11RasterizerState> m_rasterizerState;
		ComPtr<ID3D11BlendState> m_blendState;
		std::tuple< ComPtr<ID3D11Buffer>, UINT, UINT, UINT > m_vertexBuffer; // Buffer, Stride, Offset, StartLocation
	};

	std::optional<PersistentData> m_persistentData;
	std::optional<VolatileData> m_volatileData;
};

};