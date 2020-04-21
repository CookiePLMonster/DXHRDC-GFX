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
	bool colorGradingEnabled = true;
	int bloomType = 1; // 0 - stock, 1 - DXHR
	int lightingType = 2; // 0 - stock, 1 - stock fixed, 2 - DXHR

	float colorGradingAttributes[5][4] = {
		// Those are duplicated in imgui code too
		{ 0.85f,  0.75f,  1.25f },
		{ 0.25098f,  0.31373f,  0.28235f },
		{ 0.60392f,  0.52627f,  0.4098f },
		{ 0.52941f,  0.52941f,  0.52941f },
		{ 1.0f,  0.0f,  0.7f,  0.7f }
	};
};

extern Settings SETTINGS;

// Color grading presets
extern const float COLOR_GRADING_PRESETS[3][4][4];

int GetSelectedPreset( float attribs[4][4] );

};