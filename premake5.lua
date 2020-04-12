workspace "DXHRDC-GFX"
	platforms { "Win32" }

project "DXHRDC-GFX"
	kind "SharedLib"
	targetextension ".asi"
	language "C++"

	include "source/VersionInfo.lua"
	files { "**/MemoryMgr.h", "**/Patterns.*", "**/HookInit.hpp" }


workspace "*"
	configurations { "Debug", "Release", "Master" }
	location "build"

	vpaths { ["Headers/*"] = "source/**.h",
			["Sources/*"] = { "source/**.c", "source/**.cpp" },
			["Resources"] = "source/**.rc"
	}

	files { "source/*.h", "source/*.cpp", "source/resources/*.rc", "source/wil/*", "source/*.def",
 			"source/effects/*" }

	-- Disable exceptions in WIL
	defines { "WIL_SUPPRESS_EXCEPTIONS" }

	cppdialect "C++17"
	staticruntime "on"
	buildoptions { "/sdl" }
	warnings "Extra"

	-- Automated defines for resources
	defines { "rsc_Extension=\"%{prj.targetextension}\"",
			"rsc_Name=\"%{prj.name}\"" }

filter "configurations:Debug"
	defines { "DEBUG" }
	runtime "Debug"

 filter "configurations:Master"
	defines { "NDEBUG" }
	symbols "Off"

filter "configurations:not Debug"
	optimize "Speed"
	functionlevellinking "on"
	flags { "LinkTimeOptimization" }

filter { "platforms:Win32" }
	system "Windows"
	architecture "x86"

filter { "platforms:Win64" }
	system "Windows"
	architecture "x86_64"

filter { "toolset:*_xp"}
	buildoptions { "/Zc:threadSafeInit-" }

filter { "toolset:not *_xp"}
	buildoptions { "/permissive-" }