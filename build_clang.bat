@echo off

call project.bat

set BuildFolder=built
set ExternalPath=external

if /i "%1"=="release" (
	set Config=release
	set ConfigCompilerOptions=-O2
	set ConfigLinkerOptions=-Lexternal/directxtex/release
) else (
	set Config=debug
	set ConfigCompilerOptions=-O0
	set ConfigLinkerOptions=-debug -Lexternal/directxtex/debug
)

if %GfxApi% == D3D12 (
	set PrebuildFile=prebuilt\win32_d3d12_prebuild.obj
	set CompileFile=source\win32_d3d12_main.cpp
) else (
	set PrebuildFile=prebuilt\win32_d3d11_prebuild.obj
	set CompileFile=source\win32_d3d11_main.cpp
)

set CommonCompilerDefines=-D%GfxApi%

set CommonCompilerFlags=%ConfigCompilerOptions% -Wall -Werror %CommonCompilerDefines% -I%ExternalPath% -I%ExternalPath%/imgui -Isource
set CommonLinkerFlags=%ConfigLinkerOptions% -Wl,--subsystem,windows -L"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64" %GfxApi%.lib d3dcompiler.lib dxguid.lib dxgi.lib directxtex.lib ole32.lib %PrebuildFile%

if not exist %BuildFolder%\ mkdir %BuildFolder%

echo Compiling (clang): renderland.exe, Config: %Config%, GfxApi: %GfxApi%

clang++.exe %CommonCompilerFlags% %CompileFile% -o%BuildFolder%/%ProjectExe% %CommonLinkerFlags%
