#include "Lighting.h"

#include <cstdint>

#include "../wil/resource.h"

#include "Lighting_shader.h"

void Effects::Lighting::CreateAlternatePixelShader(ID3D11PixelShader * shader, const void * bytecode, SIZE_T length)
{
	static constexpr uint32_t LIGHTING_SHADER1_HASH[4] = { 0x4abe618c, 0xa282fa5b, 0xdcde9b8b, 0xaf4aa8fb };
	static constexpr uint32_t LIGHTING_SHADER2_HASH[4] = { 0x65ae0cbf, 0x89721070, 0x6078754d, 0xa3a24d48 };
	static constexpr uint32_t LIGHTING_SHADER3_HASH[4] = { 0x2ede696f, 0x36c567e9, 0xaacac074, 0xb5f3ad15 };
	static constexpr uint32_t LIGHTING_SHADER4_HASH[4] = { 0x4ee86a1e, 0xbf1eba8f, 0x48e4cf30, 0x635dc3f9 };

	if ( length >= 4 + sizeof(LIGHTING_SHADER1_HASH) )
	{
		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, LIGHTING_SHADER1_HASH, sizeof(LIGHTING_SHADER1_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::LightingShader1;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

			// Create an alternate shader
			ComPtr<ID3D11PixelShader> alternateShader;
			if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING1_PS_BYTECODE, sizeof(LIGHTING1_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
			{
				shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
			}
			return;
		}

		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, LIGHTING_SHADER2_HASH, sizeof(LIGHTING_SHADER2_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::LightingShader2;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

			// Create an alternate shader
			ComPtr<ID3D11PixelShader> alternateShader;
			if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING2_PS_BYTECODE, sizeof(LIGHTING2_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
			{
				shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
			}
			return;
		}

		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, LIGHTING_SHADER3_HASH, sizeof(LIGHTING_SHADER3_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::LightingShader3;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

			// Create an alternate shader
			ComPtr<ID3D11PixelShader> alternateShader;
			if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING3_PS_BYTECODE, sizeof(LIGHTING3_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
			{
				shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
			}
			return;
		}

		if ( memcmp( reinterpret_cast<const uint8_t*>(bytecode) + 4, LIGHTING_SHADER4_HASH, sizeof(LIGHTING_SHADER4_HASH) ) == 0 )
		{
			ResourceMetadata resource;
			resource.m_type = ResourceMetadata::Type::LightingShader4;
			shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );

			// Create an alternate shader
			ComPtr<ID3D11PixelShader> alternateShader;
			if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING4_PS_BYTECODE, sizeof(LIGHTING4_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
			{
				shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
			}
			return;
		}
	}
}

bool Effects::Lighting::OnDrawIndexed(ID3D11DeviceContext* context, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	if ( m_swapSRVs )
	{
		// Swap SRV0 and SRV1 around, then restore
		ID3D11ShaderResourceView* resources[2]; // Warning - raw pointers!
		context->PSGetShaderResources( 0, _countof(resources), resources );
		auto restore = wil::scope_exit([&] {
			context->PSSetShaderResources( 0, _countof(resources), resources );
			for ( auto* r : resources )
			{
				r->Release();
			}
		});

		ID3D11ShaderResourceView* const swappedResources[2] = { resources[1], resources[0] }; // Warning - raw pointers!
		context->PSSetShaderResources( 0, _countof(swappedResources), swappedResources );

		context->DrawIndexed( IndexCount, StartIndexLocation, BaseVertexLocation );
		return true;
	}
	return false;
}

ComPtr<ID3D11PixelShader> Effects::Lighting::BeforePixelShaderSet(ID3D11DeviceContext* context, ID3D11PixelShader* shader)
{
	ComPtr<ID3D11PixelShader> result(shader);
	m_swapSRVs = false;

	int keyState = KeyToggled(VK_F4, 0, 2);

	if ( keyState == 0 ) return result;

	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( (meta.m_type == ResourceMetadata::Type::LightingShader1 || meta.m_type == ResourceMetadata::Type::LightingShader4) ||
			 (keyState == 2 && (meta.m_type == ResourceMetadata::Type::LightingShader2 || meta.m_type == ResourceMetadata::Type::LightingShader3)) )
		{
			ComPtr<ID3D11PixelShader> replacedShader;
			size = sizeof(ID3D11PixelShader*);
			if ( SUCCEEDED(shader->GetPrivateData(GUID_AlternateResource, &size, replacedShader.GetAddressOf())) )
			{
				result = std::move(replacedShader);
				m_swapSRVs = meta.m_type == ResourceMetadata::Type::LightingShader3;
			}
		}
	}

	return result;
}
