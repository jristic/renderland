@echo off

call project.bat

set BuildFolder=built
set ExternalPath=external

if /i "%1"=="release" (
	set Config=release
	set ConfigCompilerOptions=/MT /O2 /Oi /Oy /GL /wd4189
	set ConfigLinkerOptions=/opt:ref /libpath:external/directxtex/release
) else (
	set Config=debug
	set ConfigCompilerOptions=/MTd /Od
	set ConfigLinkerOptions=/opt:noref /debug /libpath:external/directxtex/debug
)

if %GfxApi% == D3D12 (
	set PrebuildFile=prebuilt\win32_d3d12_prebuild.obj
	set CompileFile=source\win32_d3d12_main.cpp
) else (
	set PrebuildFile=prebuilt\win32_d3d11_prebuild.obj
	set CompileFile=source\win32_d3d11_main.cpp
)

set CommonCompilerDefines=/D_CRT_SECURE_NO_WARNINGS %ProjectPlatformDefines%

set CommonCompilerFlags=%ConfigCompilerOptions% /nologo /fp:fast /Gm- /GR- /EHsc /WX /W4 /FC /Z7 %CommonCompilerDefines% /I%ExternalPath% /I%ExternalPath%/imgui /Fo%BuildFolder%\
set CommonLinkerFlags=%ConfigLinkerOptions% /incremental:no /subsystem:windows %ProjectPlatformLinkLibs% d3dcompiler.lib dxguid.lib dxgi.lib directxtex.lib ole32.lib %PrebuildFile%

if not exist %BuildFolder%\ mkdir %BuildFolder%

echo Compiling: renderland.exe, Config: %Config%, GfxApi: %GfxApi%

cl.exe %CommonCompilerFlags% %CompileFile% /Fe%BuildFolder%/%ProjectExe% /link %CommonLinkerFlags%
