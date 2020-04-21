#include <Windows.h>
#include "Metadata.h"

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