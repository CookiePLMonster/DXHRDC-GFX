#include <Windows.h>
#include "Metadata.h"

#include <stdio.h>

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
