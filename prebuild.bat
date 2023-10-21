@echo off

call project.bat

set BuildFolder=prebuilt
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

set CommonCompilerDefines=/D_CRT_SECURE_NO_WARNINGS %ProjectPlatformDefines%

set CommonCompilerFlags=%ConfigCompilerOptions% /nologo /fp:fast /Gm- /GR- /EHsc /WX /W4 /FC /Z7 %CommonCompilerDefines% /I%ExternalPath% /I%ExternalPath%/imgui

if not exist %BuildFolder%\ mkdir %BuildFolder%

if %GfxApi% == D3D12 (
	set CompileFile=win32_d3d12_prebuild
) else (
	set CompileFile=win32_d3d11_prebuild
)

echo Compiling: %CompileFile%.obj, Config: %Config%

cl.exe /c %CommonCompilerFlags% source/%CompileFile%.cpp /Fo%BuildFolder%\%CompileFile%.obj
