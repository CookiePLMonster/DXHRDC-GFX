#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

HMODULE (WINAPI **orgLoadLibraryA)(LPCSTR lpLibFileName);
HMODULE WINAPI LoadLibraryA_SelfReference( LPCSTR lpLibFileName )
{
	if ( _stricmp( lpLibFileName, "d3d11.dll" ) == 0 )
	{
		// Self reference
		HMODULE lib;
		GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(&LoadLibraryA_SelfReference), &lib );
		return lib;
	}

	return (*orgLoadLibraryA)( lpLibFileName );
}
static auto* const pLoadLibraryA_SelfReference = LoadLibraryA_SelfReference;

void OnInitializeHook()
{
	using namespace Memory::VP;
	using namespace hook;

	// Make the game use this module instead of d3d11.dll when asked for it
	{
		auto loadLibrary = get_pattern<decltype(LoadLibraryA_SelfReference)**>( "8B 3D ? ? ? ? 68 ? ? ? ? FF D7", 2 );
		orgLoadLibraryA = *loadLibrary;
		Patch( loadLibrary, &pLoadLibraryA_SelfReference );
	}
}
