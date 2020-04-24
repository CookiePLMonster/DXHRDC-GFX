#include "Utils/MemoryMgr.h"
#include "Utils/Patterns.h"

#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

wchar_t wcModulePath[MAX_PATH];
static HMODULE hDLLModule;

HMODULE WINAPI LoadLibraryA_SelfReference( LPCSTR /*lpLibFileName*/ )
{
	HMODULE lib;
	GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(&LoadLibraryA_SelfReference), &lib );
	return lib;
}
static auto* const pLoadLibraryA_SelfReference = LoadLibraryA_SelfReference;

void OnInitializeHook()
{
	GetModuleFileNameW(hDLLModule, wcModulePath, _countof(wcModulePath) - 3); // Minus max required space for extension
	PathRenameExtensionW(wcModulePath, L".ini");

	using namespace Memory::VP;
	using namespace hook;

	// Make the game use this module instead of dxgi.dll/d3d11.dll when asked for it
	{
		auto loadLibrary = get_pattern<decltype(LoadLibraryA_SelfReference)**>( "8B 3D ? ? ? ? 68 ? ? ? ? FF D7", 2 );
		Patch( loadLibrary, &pLoadLibraryA_SelfReference );
	}
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if ( fdwReason == DLL_PROCESS_ATTACH )
	{
		hDLLModule = hinstDLL;
	}
	return TRUE;
}
