#include "Bloom.h"

#include <cstdint>

#include "Bloom_shader.h"

void Effects::Bloom::CreateAlternatePixelShader( ID3D11PixelShader* shader, const void* bytecode, SIZE_T length )
{
	static constexpr uint32_t BLOOM_SHADER1_HASH[4] = { 0x1e94a642, 0x771834d4, 0xf7c2a424, 0x583be5c8 };
	static constexpr uint32_t BLOOM_SHADER2_HASH[4] = { 0xaf000e64, 0x2766c2fc, 0xe3aa8a2c, 0x2f0ee3af };
	static constexpr uint32_t BLOOM_SHADER4_HASH[4] = { 0x32976b21, 0x3bec6292, 0x89f5d21c, 0xed6bd65f };

	if ( length >= 4 + sizeof(BLOOM_SHADER1_HASH) )
	{
		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, BLOOM_SHADER1_HASH, sizeof(BLOOM_SHADER1_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::BloomShader1;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

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

		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, BLOOM_SHADER2_HASH, sizeof(BLOOM_SHADER2_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::BloomShader2;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

			// Alternate shader is not used instead of this one during the first draw, but it is during the following draw with the same shader
			m_device->CreatePixelShader( BLOOM3_PS_BYTECODE, sizeof(BLOOM3_PS_BYTECODE), nullptr, m_bloom3PS.ReleaseAndGetAddressOf() );
			return;
		}

		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, BLOOM_SHADER4_HASH, sizeof(BLOOM_SHADER4_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::BloomShader4;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

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
	}

	// Special handling for blur merger shader, because it's also used by ColorGrading
	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( meta.m_type == ResourceMetadata::Type::BloomMergerShader )
		{
			// Create an alternate shader and also annotate it as a blur merger shader
			ComPtr<ID3D11PixelShader> alternateShader;
			if ( SUCCEEDED(m_device->CreatePixelShader( BLOOM_MERGER_PS_BYTECODE, sizeof(BLOOM_MERGER_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
			{
				shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
				alternateShader->SetPrivateData( __uuidof(meta), sizeof(meta), &meta );
			}
		}
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

void Effects::Bloom::BeforeDraw( ID3D11DeviceContext* context )
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
		ID3D11ShaderResourceView* v[3];
		context->PSGetShaderResources( 0, 3, v );
		ComPtr<ID3D11ShaderResourceView> views[3];
		views[0].Attach( v[0] );
		views[1].Attach( v[1] );
		views[2].Attach( v[2] );

		// SRV2 goes to SRV0, SRV0 goes to SRV1
		ID3D11ShaderResourceView* reboundSRV[] = { views[2].Get(), views[0].Get() };
		context->PSSetShaderResources( 0, 2, reboundSRV );

		ComPtr<ID3D11Buffer> cb;
		context->PSGetConstantBuffers( 5, 1, cb.GetAddressOf() );

		// CB5 goes to CB3
		context->PSSetConstantBuffers( 3, 1, cb.GetAddressOf() );
	}
}

void Effects::Bloom::AfterDraw( ID3D11DeviceContext* /*context*/ )
{
	if ( m_state == State::Bloom2Set )
	{
		m_state = State::Bloom2Drawn;
	}
}
