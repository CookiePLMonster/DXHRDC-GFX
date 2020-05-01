#include "Bloom.h"

#include <cstdint>

#include "../wil/resource.h"

#include "Bloom_shader.h"

void Effects::Bloom::CreateAlternatePixelShader( ID3D11PixelShader* shader )
{
	const ResourceMetadata::Type shaderType = GetPixelShaderAnnotation( shader ).m_type;

	if ( shaderType == ResourceMetadata::Type::BloomShader1 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( BLOOM1_PS_BYTECODE, sizeof(BLOOM1_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );

			// CB used by this shader
			const float data[128] = { 1.5f, 0.0f, 0.0f, 0.0f }; // Only 16 bytes are used but shader was defined to use 512 bytes

			D3D11_BUFFER_DESC cbDesc {};
			cbDesc.ByteWidth = sizeof(data);
			cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

			D3D11_SUBRESOURCE_DATA initialData {};
			initialData.pSysMem = data;
			m_device->CreateBuffer( &cbDesc, &initialData, m_shader1CB.ReleaseAndGetAddressOf() );
		}
		return;
	}

	if ( shaderType == ResourceMetadata::Type::BloomShader2 )
	{
		// Alternate shader is not used instead of this one during the first draw, but it is during the following draw with the same shader
		m_device->CreatePixelShader( BLOOM3_PS_BYTECODE, sizeof(BLOOM3_PS_BYTECODE), nullptr, m_bloom3PS.ReleaseAndGetAddressOf() );
		return;
	}

	if ( shaderType == ResourceMetadata::Type::BloomShader4 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( BLOOM4_PS_BYTECODE, sizeof(BLOOM4_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );

			// CB used by this shader
			const float data[128] = { 1.5f, 1.5f, 1.0f, 0.0f }; // Only 16 bytes are used but shader was defined to use 512 bytes

			D3D11_BUFFER_DESC cbDesc {};
			cbDesc.ByteWidth = sizeof(data);
			cbDesc.Usage = D3D11_USAGE_IMMUTABLE;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

			D3D11_SUBRESOURCE_DATA initialData {};
			initialData.pSysMem = data;
			m_device->CreateBuffer( &cbDesc, &initialData, m_shader4CB.ReleaseAndGetAddressOf() );
		}
		return;
	}

	if ( shaderType == ResourceMetadata::Type::BloomMergerShader )
	{
		// Create an alternate shader and also annotate it as a blur merger shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( BLOOM_MERGER_PS_BYTECODE, sizeof(BLOOM_MERGER_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
			AnnotatePixelShader( alternateShader.Get(), shaderType, true );
		}
		return;
	}
}

ComPtr<ID3D11PixelShader> Effects::Bloom::BeforePixelShaderSet( ID3D11DeviceContext* context, ID3D11PixelShader* shader )
{
	ComPtr<ID3D11PixelShader> result(shader);

	if ( SETTINGS.bloomType == 0 ) return result;

	m_state = State::Initial;
	
	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( meta.m_type == ResourceMetadata::Type::BloomShader1 ) // Bloom shader 1 - replace shader and bind a custom constant buffer
		{
			ComPtr<ID3D11PixelShader> replacedShader;
			size = sizeof(ID3D11PixelShader*);
			if ( SUCCEEDED(shader->GetPrivateData(GUID_AlternateResource, &size, replacedShader.GetAddressOf())) )
			{
				result = std::move(replacedShader);

				context->PSSetConstantBuffers( 3, 1, m_shader1CB.GetAddressOf() );
			}
		}
		else if ( meta.m_type == ResourceMetadata::Type::BloomShader2 ) // Bloom shader 2 - don't replace, but advance the state machine
		{
			m_state = State::Bloom2Set;
		}
		else if ( meta.m_type == ResourceMetadata::Type::BloomShader4 ) // Bloom shader 4 - replace shader and bind a custom constant buffer
		{
			ComPtr<ID3D11PixelShader> replacedShader;
			size = sizeof(ID3D11PixelShader*);
			if ( SUCCEEDED(shader->GetPrivateData(GUID_AlternateResource, &size, replacedShader.GetAddressOf())) )
			{
				result = std::move(replacedShader);

				context->PSSetConstantBuffers( 3, 1, m_shader4CB.GetAddressOf() );
			}
		}
		else if ( meta.m_type == ResourceMetadata::Type::BloomMergerShader ) // Bloom merger - replace shader, then rebind inputs before drawing
		{
			ComPtr<ID3D11PixelShader> replacedShader;
			size = sizeof(ID3D11PixelShader*);
			if ( SUCCEEDED(shader->GetPrivateData(GUID_AlternateResource, &size, replacedShader.GetAddressOf())) )
			{
				result = std::move(replacedShader);

				m_state = State::MergerPSFound;
			}	
		}
	}
	return result;
}

bool Effects::Bloom::OnDraw( ID3D11DeviceContext* context, UINT VertexCount, UINT StartVertexLocation )
{
	if ( m_state == State::Bloom2Drawn )
	{
		m_state = State::Initial;

		// Replace with an alternate bloom3 shader
		context->PSSetShader( m_bloom3PS.Get(), nullptr, 0 );
	}
	else if ( m_state == State::MergerPSFound )
	{
		m_state = State::Initial;

		// Rebind inputs to match the modified shader
		ID3D11ShaderResourceView* view[3]; // Warning - raw pointers!
		context->PSGetShaderResources( 0, 3, view );
		auto releaseSRV = wil::scope_exit([&] {
			for ( auto* r : view )
			{
				if ( r != nullptr )
				{
					r->Release();
				}
			}
		});

		// SRV2 goes to SRV0, SRV0 goes to SRV1
		ID3D11ShaderResourceView* const reboundSRV[] = { view[2], view[0] };
		context->PSSetShaderResources( 0, _countof(reboundSRV), reboundSRV );

		ID3D11Buffer* cb[3]; // Warning - raw pointers!
		context->PSGetConstantBuffers( 3, 3, cb );
		auto releaseCB = wil::scope_exit([&] {
			for ( auto* r : cb )
			{
				if ( r != nullptr )
				{
					r->Release();
				}
			}
		});

		// CB5 goes to CB3
		context->PSSetConstantBuffers( 3, 1, &cb[2] );

		// Rebind the original resources after the draw
		auto restore = wil::scope_exit([&] {
			context->PSSetConstantBuffers( 3, 1, &cb[0] );

			context->PSSetShaderResources( 0, _countof(view), view );
		});

		context->Draw( VertexCount, StartVertexLocation );
		return true;
	}
	else if ( m_state == State::Bloom2Set )
	{
		m_state = State::Bloom2Drawn;
	}

	return false;
}

