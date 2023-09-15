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

echo Compiling: win32_prebuilt.obj, Config: %Config%

cl.exe /c %CommonCompilerFlags% source/win32_prebuild.cpp /Fo%BuildFolder%\win32_prebuilt.obj
