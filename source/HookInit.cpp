#define WINVER 0x0502
#define _WIN32_WINNT 0x0502

#include <windows.h>

#define HOOKED_FUNCTION GetCommandLineA
#define HOOKED_LIBRARY "KERNEL32.DLL"

#include "Utils/HookInit.hpp"
