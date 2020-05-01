#include "Lighting.h"

#include <cstdint>

#include "../wil/resource.h"

#include "Lighting_shader.h"

void Effects::Lighting::CreateAlternatePixelShader(ID3D11PixelShader* shader)
{
	const ResourceMetadata::Type shaderType = GetPixelShaderAnnotation( shader ).m_type;

	if ( shaderType == ResourceMetadata::Type::LightingShader1 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING1_PS_BYTECODE, sizeof(LIGHTING1_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
		}
		return;
	}

	if ( shaderType == ResourceMetadata::Type::LightingShader2 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING2_PS_BYTECODE, sizeof(LIGHTING2_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
		}
		return;
	}

	if ( shaderType == ResourceMetadata::Type::LightingShader3 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING3_PS_BYTECODE, sizeof(LIGHTING3_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
		}
		return;
	}

	if ( shaderType == ResourceMetadata::Type::LightingShader4 )
	{
		// Create an alternate shader
		ComPtr<ID3D11PixelShader> alternateShader;
		if ( SUCCEEDED(m_device->CreatePixelShader( LIGHTING4_PS_BYTECODE, sizeof(LIGHTING4_PS_BYTECODE), nullptr, alternateShader.GetAddressOf() )) )
		{
			shader->SetPrivateDataInterface( GUID_AlternateResource, alternateShader.Get() );
		}
		return;
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
				if ( r != nullptr )
				{
					r->Release();
				}
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

	if ( SETTINGS.lightingType == 0 ) return result;

	ResourceMetadata meta;
	UINT size = sizeof(meta);
	if ( SUCCEEDED(shader->GetPrivateData(__uuidof(meta), &size, &meta)) )
	{
		if ( (meta.m_type == ResourceMetadata::Type::LightingShader1 || meta.m_type == ResourceMetadata::Type::LightingShader4) ||
			 (SETTINGS.lightingType == 2 && (meta.m_type == ResourceMetadata::Type::LightingShader2 || meta.m_type == ResourceMetadata::Type::LightingShader3)) )
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
