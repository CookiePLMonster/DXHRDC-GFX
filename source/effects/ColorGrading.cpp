#include "ColorGrading.h"

#include <cstdint>
#include <utility>

#include "../wil/resource.h"

#include "ColorGrading_shader.h"

#define DEBUG_COLOR_GRADING_CALLS 0

#if DEBUG_COLOR_GRADING_CALLS
#include <d3d11_1.h>
#endif

Effects::ColorGrading::ColorGrading(ID3D11Device* device)
	: m_device(device)
{
	m_device->CreatePixelShader( COLOR_GRADING_PS_BYTECODE, sizeof(COLOR_GRADING_PS_BYTECODE), nullptr, m_pixelShader.GetAddressOf() );

	D3D11_BUFFER_DESC cbDesc {};
	cbDesc.ByteWidth = 512;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	m_device->CreateBuffer( &cbDesc, nullptr, m_constantBuffer.GetAddressOf() );
}

void Effects::ColorGrading::OnPixelShaderSet(ID3D11PixelShader* shader)
{
	if ( !SETTINGS.colorGradingEnabled ) return;

	const ResourceMetadata::Type shaderType = GetPixelShaderAnnotation( shader ).m_type;

	if ( shaderType == ResourceMetadata::Type::BloomMergerShader )
	{
		m_state = State::MergerCallFound;
		return;
	}

	if ( m_state == State::MergerCallFound )
	{
		m_state = State::Initial; // Reset in case the required shader was "found" but changed before it was used
		return;
	}

	if ( m_state == State::ResourcesGathered )
	{
		// If setting to use Edge AA, ignore blend state changes
		m_volatileData->m_edgeAADetected = shaderType == ResourceMetadata::Type::EdgeAA;
	}
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
			m_persistentData = std::make_optional<PersistentData>();
		}
	}
}

void Effects::ColorGrading::BeforeOMSetBlendState(ID3D11DeviceContext* context, ID3D11BlendState* pBlendState)
{
	if ( m_state == State::ResourcesGathered )
	{
		// If setting to a different blend state to what we saved, draw
		if ( !m_volatileData->m_edgeAADetected && m_volatileData->m_blendState.Get() != pBlendState )
		{
			// Draw to current RTV0
			ComPtr<ID3D11RenderTargetView> curRTV;
			ComPtr<ID3D11DepthStencilView> curDSV;
			context->OMGetRenderTargets( 1, curRTV.GetAddressOf(), curDSV.GetAddressOf() );
			auto restoreRTV = wil::scope_exit([&] {
				context->OMSetRenderTargets( 1, curRTV.GetAddressOf(), curDSV.Get() );
			});

#if DEBUG_COLOR_GRADING_CALLS
			ComPtr<ID3DUserDefinedAnnotation> annotation;
			if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(annotation.GetAddressOf()))))
			{
				annotation->SetMarker( L"BeforeOMSetBlendState" );
			}
#endif

			ComPtr<ID3D11Resource> curRT;
			curRTV->GetResource( curRT.GetAddressOf() );
			DrawColorFilter( context, curRT );
		}
	}
}

void Effects::ColorGrading::BeforeOMSetRenderTargets(ID3D11DeviceContext* context, UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
	if ( m_state == State::ResourcesGathered )
	{
		// If unbinding the RTV, save it in case we need to render using our special case for additional blur
		if ( NumViews == 0 && pDepthStencilView == nullptr )
		{
			context->OMGetRenderTargets( 1, m_volatileData->m_lastUnboundRTV.ReleaseAndGetAddressOf(), nullptr );
			return;
		}

		// If setting a single render target to something smaller than the gathered render target, draw
		if ( NumViews == 1 && ppRenderTargetViews != nullptr && pDepthStencilView == nullptr )
		{
			ComPtr<ID3D11Resource> curRT;
			ppRenderTargetViews[0]->GetResource( curRT.GetAddressOf() );

			ComPtr<ID3D11Texture2D> curTexture;
			curRT.As(&curTexture);

			D3D11_TEXTURE2D_DESC desc;
			curTexture->GetDesc( &desc );

			if ( desc.Width < std::get<1>(m_persistentData->m_tempRT) && desc.Height < std::get<2>(m_persistentData->m_tempRT) )
			{
				// Draw to "last" RTV0
				// No need to save/restore render targets as they will be overwritten
#if DEBUG_COLOR_GRADING_CALLS
				ComPtr<ID3DUserDefinedAnnotation> annotation;
				if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(annotation.GetAddressOf()))))
				{
					annotation->SetMarker( L"BeforeOMSetRenderTargets" );
				}
#endif

				m_volatileData->m_lastUnboundRTV->GetResource( curRT.ReleaseAndGetAddressOf() );
				DrawColorFilter( context, curRT );
			}
			return;
		}
	}
}

void Effects::ColorGrading::ClearState()
{
	m_persistentData.reset();
	m_volatileData.reset();
	m_state = State::Initial;
}

void Effects::ColorGrading::DrawColorFilter(ID3D11DeviceContext* context, const ComPtr<ID3D11Resource>& target)
{
	m_state = State::Initial;

	ComPtr<ID3D11Texture2D> curTexture;
	target.As(&curTexture);

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
	if ( m_persistentData->m_lastOutputRT == nullptr || m_persistentData->m_lastOutputRT != target )
	{
		m_persistentData->m_lastOutputRT = target;
		m_device->CreateShaderResourceView( target.Get(), nullptr, m_persistentData->m_lastOutputSRV.ReleaseAndGetAddressOf() );
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

#if DEBUG_COLOR_GRADING_CALLS
	ComPtr<ID3DUserDefinedAnnotation> annotation;
	auto endEvent = wil::scope_exit([&] {
		if (annotation)
		{
			annotation->EndEvent();
		}
	});


	if (SUCCEEDED(context->QueryInterface(IID_PPV_ARGS(annotation.GetAddressOf()))))
	{
		annotation->BeginEvent( L"DrawColorFilter" );
	}
#endif

	context->VSSetShader( m_volatileData->m_vertexShader.Get(), nullptr, 0 );
	context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
	context->IASetInputLayout( m_volatileData->m_inputLayout.Get() );
	context->RSSetState( m_volatileData->m_rasterizerState.Get() );

	context->OMSetRenderTargets( 1, m_persistentData->m_tempRTV.GetAddressOf(), nullptr );

	context->IASetVertexBuffers( 0, 1, std::get<0>(m_volatileData->m_vertexBuffer).GetAddressOf(),
			&std::get<1>(m_volatileData->m_vertexBuffer), &std::get<2>(m_volatileData->m_vertexBuffer) );
	context->PSSetConstantBuffers( 5, 1, m_constantBuffer.GetAddressOf() );
	context->PSSetShaderResources( 0, 1, m_persistentData->m_lastOutputSRV.GetAddressOf() );

	if ( std::exchange(SETTINGS.colorGradingDirty, false) )
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		if ( SUCCEEDED(context->Map( m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped )) )
		{
			memcpy( mapped.pData, SETTINGS.colorGradingAttributes, sizeof(SETTINGS.colorGradingAttributes) );
			context->Unmap( m_constantBuffer.Get(), 0 );
		}
	}

	context->Draw( 6, std::get<3>(m_volatileData->m_vertexBuffer) );
	context->CopyResource( target.Get(), std::get<0>(m_persistentData->m_tempRT).Get() );

	m_volatileData.reset();
}

