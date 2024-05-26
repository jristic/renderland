@echo off

call project.bat

set BuildFolder=prebuilt
set ExternalPath=external

if /i "%1"=="release" (
	set Config=release
	set ConfigCompilerOptions=-O2
	set ConfigLinkerOptions=-Lexternal/directxtex/release
) else (
	set Config=debug
	set ConfigCompilerOptions=-O0 --debug -D_DEBUG
	set ConfigLinkerOptions=--debug -Lexternal/directxtex/debug
)

set CommonCompilerDefines=-D%GfxApi%

set CommonCompilerFlags=%ConfigCompilerOptions% -Wall -Werror %CommonCompilerDefines% -I%ExternalPath% -I%ExternalPath%/imgui

if not exist %BuildFolder%\ mkdir %BuildFolder%

if %GfxApi% == D3D12 (
	set CompileFile=win32_d3d12_prebuild
) else (
	set CompileFile=win32_d3d11_prebuild
)

echo Compiling: %CompileFile%.obj, Config: %Config%

clang++.exe -c %CommonCompilerFlags% source/%CompileFile%.cpp -o%BuildFolder%\%CompileFile%.obj
