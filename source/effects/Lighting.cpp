#include "Lighting.h"

#include <cstdint>

#include "Lighting_shader.h"

void Effects::Lighting::CreateAlternatePixelShader(ID3D11PixelShader * shader, const void * bytecode, SIZE_T length)
{
	static constexpr uint32_t LIGHTING_SHADER1_HASH[4] = { 0x4abe618c, 0xa282fa5b, 0xdcde9b8b, 0xaf4aa8fb };
	static constexpr uint32_t LIGHTING_SHADER2_HASH[4] = { 0x65ae0cbf, 0x89721070, 0x6078754d, 0xa3a24d48 };

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
	}
}

ComPtr<ID3D11PixelShader> Effects::Lighting::BeforePixelShaderSet(ID3D11DeviceContext* context, ID3D11PixelShader* shader)
{
	ComPtr<ID3D11PixelShader> result(shader);

	if ( GetAsyncKeyState(VK_F4) & 0x8000 ) return result;

	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( meta.m_type == ResourceMetadata::Type::LightingShader1 || meta.m_type == ResourceMetadata::Type::LightingShader2 )
		{
			ComPtr<ID3D11PixelShader> replacedShader;
			size = sizeof(ID3D11PixelShader*);
			if ( SUCCEEDED(shader->GetPrivateData(GUID_AlternateResource, &size, replacedShader.GetAddressOf())) )
			{
				result = std::move(replacedShader);
			}
		}
	}

	return result;
}
