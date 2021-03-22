@echo off

call project.bat

set BuildFolder=built

set ExternalPath="external"
set CommonCompilerFlags=/MTd /nologo /fp:fast /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE /I%ExternalPath% /Fo%BuildFolder%\
set CommonLinkerFlags=/incremental:no /opt:ref /subsystem:windows d3d11.lib d3dcompiler.lib

if not exist %BuildFolder%\ mkdir %BuildFolder%

cl.exe %CommonCompilerFlags% source/win32_main.cpp /Fe%BuildFolder%/%ProjectExe% /link %CommonLinkerFlags%
