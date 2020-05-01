#include <Windows.h>
#include "Metadata.h"

#include <stdio.h>
#include <cstdint>
#include <utility>
#include <array>

extern wchar_t wcModulePath[MAX_PATH];

Effects::Settings Effects::SETTINGS;

const float Effects::COLOR_GRADING_PRESETS[3][4][4] = {
	{
		{ 0.85f,  0.75f,  1.25f },
		{ 0.25098f,  0.31373f,  0.28235f },
		{ 0.60392f,  0.52627f,  0.4098f },
		{ 0.52941f,  0.52941f,  0.52941f }
	},

	{
		{ 0.8f, 0.8f, 1.35f },
		{ 0.37255f, 0.43922f, 0.54902f },
		{ 0.62745f, 0.57647f, 0.31373f },
		{ 0.54902f, 0.51765f, 0.41569f }
	},

	{
		{ 1.0f, 0.72f, 1.4f },
		{ 0.31765f, 0.43137f, 0.40392f },
		{ 0.58824f, 0.54902f, 0.41961f },
		{ 0.54902f, 0.52157f, 0.44314f }
	},
};

const float Effects::VIGNETTE_PRESET[4] = { 1.0f,  0.0f,  0.7f,  0.7f };

void Effects::AnnotatePixelShader( ID3D11PixelShader* shader, ResourceMetadata::Type type, bool /*replacement*/ )
{
	ResourceMetadata resource;
	resource.m_type = type;
	shader->SetPrivateData( __uuidof(resource), sizeof(resource), &resource );
}

void Effects::AnnotatePixelShader( ID3D11PixelShader* shader, const void* bytecode, SIZE_T length )
{
	static constexpr std::pair< std::array<uint32_t, 4>, ResourceMetadata::Type > importantShaders[] = {
		{ { 0xf3896ba8, 0x4f0671da, 0xa690e62a, 0xc9168288 }, ResourceMetadata::Type::BloomMergerShader },
		{ { 0x1e94a642, 0x771834d4, 0xf7c2a424, 0x583be5c8 }, ResourceMetadata::Type::BloomShader1 },
		{ { 0xaf000e64, 0x2766c2fc, 0xe3aa8a2c, 0x2f0ee3af }, ResourceMetadata::Type::BloomShader2 },
		{ { 0x32976b21, 0x3bec6292, 0x89f5d21c, 0xed6bd65f }, ResourceMetadata::Type::BloomShader4 },

		{ { 0x4abe618c, 0xa282fa5b, 0xdcde9b8b, 0xaf4aa8fb }, ResourceMetadata::Type::LightingShader1 },
		{ { 0x65ae0cbf, 0x89721070, 0x6078754d, 0xa3a24d48 }, ResourceMetadata::Type::LightingShader2 },
		{ { 0x2ede696f, 0x36c567e9, 0xaacac074, 0xb5f3ad15 }, ResourceMetadata::Type::LightingShader3 },
		{ { 0x4ee86a1e, 0xbf1eba8f, 0x48e4cf30, 0x635dc3f9 }, ResourceMetadata::Type::LightingShader4 },
	};

	if ( length >= 4 + 16 )
	{
		const uint32_t* hash = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(bytecode) + 4);
		for ( const auto& sh : importantShaders )
		{
			if ( std::equal( sh.first.begin(), sh.first.end(), hash ) )
			{
				AnnotatePixelShader( shader, sh.second, false );
				return;
			}

		}
	}
}

auto Effects::GetPixelShaderAnnotation(ID3D11PixelShader* shader) -> ResourceMetadata
{
	ResourceMetadata result;
	UINT size = sizeof(result);
	if ( FAILED(shader->GetPrivateData(__uuidof(result), &size, &result)) )
	{
		result.m_type = ResourceMetadata::Type::None;
	}
	return result;
}

int Effects::GetSelectedPreset( float attribs[4][4] )
{
	const int numPresets = _countof( COLOR_GRADING_PRESETS );
	for ( int i = 0; i < numPresets; i++ )
	{
		if ( memcmp( attribs, COLOR_GRADING_PRESETS[i], sizeof(COLOR_GRADING_PRESETS[i]) ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

void Effects::SaveSettings()
{
	wchar_t buffer[16];

	// Basic
	swprintf_s( buffer, L"%d", SETTINGS.colorGradingEnabled );
	WritePrivateProfileStringW( L"Basic", L"EnableColorGrading", buffer, wcModulePath );

	swprintf_s( buffer, L"%d", SETTINGS.bloomType );
	WritePrivateProfileStringW( L"Basic", L"BloomStyle", buffer, wcModulePath );

	swprintf_s( buffer, L"%d", SETTINGS.lightingType );
	WritePrivateProfileStringW( L"Basic", L"LightingStyle", buffer, wcModulePath );

	// Advanced
	WritePrivateProfileStructW( L"Advanced", L"Attribs", &SETTINGS.colorGradingAttributes[0], sizeof(float) * 3, wcModulePath );
	WritePrivateProfileStructW( L"Advanced", L"Color1", &SETTINGS.colorGradingAttributes[1], sizeof(float) * 3, wcModulePath );
	WritePrivateProfileStructW( L"Advanced", L"Color2", &SETTINGS.colorGradingAttributes[2], sizeof(float) * 3, wcModulePath );
	WritePrivateProfileStructW( L"Advanced", L"Color3", &SETTINGS.colorGradingAttributes[3], sizeof(float) * 3, wcModulePath );
	WritePrivateProfileStructW( L"Advanced", L"Vignette", &SETTINGS.colorGradingAttributes[4], sizeof(float) * 4, wcModulePath );
}

void Effects::LoadSettings()
{
	SETTINGS.colorGradingEnabled = GetPrivateProfileIntW( L"Basic", L"EnableColorGrading", 1, wcModulePath );
	SETTINGS.bloomType = GetPrivateProfileIntW( L"Basic", L"BloomStyle", 1, wcModulePath );
	SETTINGS.lightingType = GetPrivateProfileIntW( L"Basic", L"LightingStyle", 1, wcModulePath );

	// If color grading fails to load, reset it all, but leave vignette separate
	if ( 
		GetPrivateProfileStructW( L"Advanced", L"Attribs", &SETTINGS.colorGradingAttributes[0], sizeof(float) * 3, wcModulePath ) == FALSE ||
		GetPrivateProfileStructW( L"Advanced", L"Color1", &SETTINGS.colorGradingAttributes[1], sizeof(float) * 3, wcModulePath ) == FALSE ||
		GetPrivateProfileStructW( L"Advanced", L"Color2", &SETTINGS.colorGradingAttributes[2], sizeof(float) * 3, wcModulePath ) == FALSE ||
		GetPrivateProfileStructW( L"Advanced", L"Color3", &SETTINGS.colorGradingAttributes[3], sizeof(float) * 3, wcModulePath ) == FALSE )
	{
		memcpy( &SETTINGS.colorGradingAttributes[0], COLOR_GRADING_PRESETS[0], sizeof(COLOR_GRADING_PRESETS[0]) );
	}

	if ( GetPrivateProfileStructW( L"Advanced", L"Vignette", &SETTINGS.colorGradingAttributes[4], sizeof(float) * 4, wcModulePath ) == FALSE )
	{
		memcpy( &SETTINGS.colorGradingAttributes[4], Effects::VIGNETTE_PRESET, sizeof(Effects::VIGNETTE_PRESET) );
	}
}
