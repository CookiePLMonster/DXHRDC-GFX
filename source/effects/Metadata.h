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
	};

	Type m_type;
};
static_assert(std::is_trivial_v<ResourceMetadata>); // Private data is memcpy'd around and destructed by freeing memory only

// {2BBE62D5-AB9D-47B1-AED2-F40C375BDD0F}
static const GUID GUID_AlternateResource = 
	{ 0x2bbe62d5, 0xab9d, 0x47b1, { 0xae, 0xd2, 0xf4, 0xc, 0x37, 0x5b, 0xdd, 0xf } };

};