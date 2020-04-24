#pragma once

#include <type_traits>

namespace Effects 
{

// Private data we attach to "interesting" resources
struct __declspec(uuid("5383C3EB-9DE8-48FC-8C88-8721759EA8E6")) ResourceMetadata
{
	// Found by hashing
	enum class Type
	{
		BloomMergerShader,
		BloomShader1, // Different in DXHR
		BloomShader2, // Same in DXHR, but holds an additional "BloomShader3" as an additional resource
		BloomShader4, // Different in DXHR

		LightingShader1,
		LightingShader2,
		LightingShader3,
		LightingShader4,
	};

	Type m_type;
};
static_assert(std::is_trivial_v<ResourceMetadata>); // Private data is memcpy'd around and destructed by freeing memory only

// {2BBE62D5-AB9D-47B1-AED2-F40C375BDD0F}
static const GUID GUID_AlternateResource = 
	{ 0x2bbe62d5, 0xab9d, 0x47b1, { 0xae, 0xd2, 0xf4, 0xc, 0x37, 0x5b, 0xdd, 0xf } };


// Global options, controlled by UI and mostly saved to INI
struct Settings
{
	// Those don't save
	bool isShown = false;
	bool colorGradingDirty = true;

	// Those save
	bool colorGradingEnabled;
	int bloomType; // 0 - stock, 1 - DXHR
	int lightingType; // 0 - stock, 1 - stock fixed, 2 - DXHR

	float colorGradingAttributes[5][4] {};
};

extern Settings SETTINGS;

// Color grading presets
extern const float COLOR_GRADING_PRESETS[3][4][4];
extern const float VIGNETTE_PRESET[4];

int GetSelectedPreset( float attribs[4][4] );

void SaveSettings();
void LoadSettings();

};