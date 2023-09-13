@echo off

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

set CommonCompilerFlags=%ConfigCompilerOptions% /nologo /fp:fast /Gm- /GR- /EHsc /WX /W4 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /I%ExternalPath% /I%ExternalPath%/imgui
set CommonLinkerFlags=%ConfigLinkerOptions% /incremental:no /subsystem:windows d3d11.lib d3dcompiler.lib dxguid.lib directxtex.lib ole32.lib

if not exist %BuildFolder%\ mkdir %BuildFolder%

echo Compiling: win32_prebuilt.obj, Config: %Config%

cl.exe /c %CommonCompilerFlags% source/win32_prebuild.cpp /Fo%BuildFolder%\win32_prebuilt.obj
