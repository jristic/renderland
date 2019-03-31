@echo off

call project.bat

set CommonCompilerFlags=/MTd /nologo /fp:fast /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /Iimgui
set CommonLinkerFlags=/incremental:no /opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

cl.exe %CommonCompilerFlags% source/win32_main.cpp /Fe%ProjectExe% /link %CommonLinkerFlags%
