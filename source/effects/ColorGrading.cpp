#include "ColorGrading.h"

#include <cstdint>
#include <utility>

#include "../wil/resource.h"

#include "ColorGrading_shader.h"

void Effects::ColorGrading::AnnotatePixelShader(ID3D11PixelShader* shader, const void* bytecode, SIZE_T length)
{
	// TODO: Factorize when we need more resources annotated
	static constexpr uint32_t BLOOM_MERGER_SHADER_HASH[4] = { 0xf3896ba8, 0x4f0671da, 0xa690e62a, 0xc9168288 };

	if ( length >= 4 + sizeof(BLOOM_MERGER_SHADER_HASH) )
	{
		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, BLOOM_MERGER_SHADER_HASH, sizeof(BLOOM_MERGER_SHADER_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::BloomMergerShader;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );
		}
	}
}

void Effects::ColorGrading::OnPixelShaderSet(ID3D11PixelShader* shader)
{
	if ( GetAsyncKeyState(VK_F2) & 0x8000 ) return;

	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( meta.m_type == ResourceMetadata::Type::BloomMergerShader )
		{
			m_state = State::MergerCallFound;
			return;
		}
	}

	if ( m_state == State::MergerCallFound ) m_state = State::Initial; // Reset in case the required shader was "found" but changed before it was used
}

void Effects::ColorGrading::BeforeDraw( ID3D11DeviceContext* context, UINT VertexCount, UINT StartVertexLocation )
{
	if ( m_state == State::MergerCallFound )
	{
		if ( VertexCount != 6 )
		{
			// Something went wrong, this draw call is not bloom postfx
			m_state = State::Initial;
			return;
		}

		m_state = State::ResourcesGathered;

		m_volatileData = std::make_optional<VolatileData>();

		context->VSGetShader( m_volatileData->m_vertexShader.GetAddressOf(), nullptr, nullptr );
		context->IAGetInputLayout( m_volatileData->m_inputLayout.GetAddressOf() );
		context->RSGetState( m_volatileData->m_rasterizerState.GetAddressOf() );
		context->OMGetBlendState( m_volatileData->m_blendState.GetAddressOf(), nullptr, nullptr );
		context->IAGetVertexBuffers( 0, 1, std::get<0>(m_volatileData->m_vertexBuffer).ReleaseAndGetAddressOf(),
					&std::get<1>(m_volatileData->m_vertexBuffer), &std::get<2>(m_volatileData->m_vertexBuffer) );
		std::get<3>(m_volatileData->m_vertexBuffer) = StartVertexLocation;

		if ( !m_persistentData.has_value() )
		{
			CreatePersistentData();
		}
	}
}

void Effects::ColorGrading::BeforeOMSetBlendState( ID3D11DeviceContext* context, ID3D11BlendState* pBlendState )
{
	if ( m_state == State::ResourcesGathered )
	{
		// If setting to a different blend state to what we saved, draw
		if ( m_volatileData->m_blendState.Get() != pBlendState )
		{
			m_state = State::Initial;

			ComPtr<ID3D11RenderTargetView> curRTV;
			ComPtr<ID3D11DepthStencilView> curDSV;
			context->OMGetRenderTargets( 1, curRTV.GetAddressOf(), curDSV.GetAddressOf() );
			auto restoreRTV = wil::scope_exit([&] {
				context->OMSetRenderTargets( 1, curRTV.GetAddressOf(), curDSV.Get() );
			});

			ComPtr<ID3D11Resource> curRT;
			curRTV->GetResource( curRT.GetAddressOf() );

			ComPtr<ID3D11Texture2D> curTexture;
			curRT.As(&curTexture);

			D3D11_TEXTURE2D_DESC desc;
			curTexture->GetDesc( &desc );

			// Recreate the temporary RT if dimensions don't match
			if ( std::get<1>(m_persistentData->m_tempRT) != desc.Width || std::get<2>(m_persistentData->m_tempRT) != desc.Height )
			{
				m_device->CreateTexture2D( &desc, nullptr, std::get<0>(m_persistentData->m_tempRT).ReleaseAndGetAddressOf() );
				m_device->CreateRenderTargetView( std::get<0>(m_persistentData->m_tempRT).Get(), nullptr, m_persistentData->m_tempRTV.ReleaseAndGetAddressOf() );

				std::get<1>(m_persistentData->m_tempRT) = desc.Width;
				std::get<2>(m_persistentData->m_tempRT) = desc.Height;
			}

			// Recreate the SRV if cached RT doesn't match
			// It should be cheap to recreate so such low effort caching should be enough
			if ( m_persistentData->m_lastOutputRT == nullptr || m_persistentData->m_lastOutputRT != curRT )
			{
				m_persistentData->m_lastOutputRT = curRT;
				m_device->CreateShaderResourceView( curRT.Get(), nullptr, m_persistentData->m_lastOutputSRV.ReleaseAndGetAddressOf() );
			}


			// Save states to restore them after drawing
			ComPtr<ID3D11VertexShader> savedVertexShader;
			ComPtr<ID3D11PixelShader> savedPixelShader;
			ComPtr<ID3D11InputLayout> savedInputLayout;
			ComPtr<ID3D11RasterizerState> savedRasterizerState;
			ComPtr<ID3D11ShaderResourceView> savedSRV;
			std::tuple< ComPtr<ID3D11Buffer>, UINT, UINT > savedInputBuffer;
			ComPtr<ID3D11Buffer> savedConstantBuffer;

			context->VSGetShader( savedVertexShader.GetAddressOf(), nullptr, nullptr );
			context->PSGetShader( savedPixelShader.GetAddressOf(), nullptr, nullptr );
			context->IAGetInputLayout( savedInputLayout.GetAddressOf() );
			context->RSGetState( savedRasterizerState.GetAddressOf() );
			context->PSGetShaderResources( 0, 1, savedSRV.GetAddressOf() );
			context->IAGetVertexBuffers( 0, 1, std::get<0>(savedInputBuffer).GetAddressOf(), &std::get<1>(savedInputBuffer), &std::get<2>(savedInputBuffer) );
			context->PSGetConstantBuffers( 5, 1, savedConstantBuffer.ReleaseAndGetAddressOf() );

			auto restore = wil::scope_exit([&] {
				context->PSSetConstantBuffers( 5, 1, savedConstantBuffer.GetAddressOf() );
				context->IASetVertexBuffers( 0, 1, std::get<0>(savedInputBuffer).GetAddressOf(), &std::get<1>(savedInputBuffer), &std::get<2>(savedInputBuffer) );
				context->PSSetShaderResources( 0, 1, savedSRV.GetAddressOf() );
				context->RSSetState( savedRasterizerState.Get() );
				context->IASetInputLayout( savedInputLayout.Get() );
				context->PSSetShader( savedPixelShader.Get(), nullptr, 0 );
				context->VSSetShader( savedVertexShader.Get(), nullptr, 0 );		
			});

			context->VSSetShader( m_volatileData->m_vertexShader.Get(), nullptr, 0 );
			context->PSSetShader( m_persistentData->m_pixelShader.Get(), nullptr, 0 );
			context->IASetInputLayout( m_volatileData->m_inputLayout.Get() );
			context->RSSetState( m_volatileData->m_rasterizerState.Get() );

			context->OMSetRenderTargets( 1, m_persistentData->m_tempRTV.GetAddressOf(), nullptr );

			context->IASetVertexBuffers( 0, 1, std::get<0>(m_volatileData->m_vertexBuffer).GetAddressOf(),
					&std::get<1>(m_volatileData->m_vertexBuffer), &std::get<2>(m_volatileData->m_vertexBuffer) );
			context->PSSetConstantBuffers( 5, 1, m_persistentData->m_constantBuffer.GetAddressOf() );
			context->PSSetShaderResources( 0, 1, m_persistentData->m_lastOutputSRV.GetAddressOf() );

			context->Draw( 6, std::get<3>(m_volatileData->m_vertexBuffer) );
			context->CopyResource( curRT.Get(), std::get<0>(m_persistentData->m_tempRT).Get() );

			m_volatileData.reset();
		}
	}
}

void Effects::ColorGrading::CreatePersistentData()
{
	m_persistentData = std::make_optional<PersistentData>();

	m_device->CreatePixelShader( COLOR_GRADING_PS_BYTECODE, sizeof(COLOR_GRADING_PS_BYTECODE), nullptr, m_persistentData->m_pixelShader.GetAddressOf() );

	static const float colorCorrection[32] = {
		0.85f,  0.75f,  1.25f,  0.0f,
		0.25098f,  0.31373f,  0.28235f,  1.0f,
		0.60392f,  0.52627f,  0.4098f,  1.1f,
		0.52941f,  0.52941f,  0.52941f,  1.0f,
		1.0f,  0.0f,  0.7f,  0.7f,
	};

	D3D11_BUFFER_DESC cbDesc {};
	cbDesc.ByteWidth = sizeof(colorCorrection);
	cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA initialData {};
	initialData.pSysMem = colorCorrection;
	m_device->CreateBuffer( &cbDesc, &initialData, m_persistentData->m_constantBuffer.GetAddressOf() );
}
