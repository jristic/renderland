@echo off

call project.bat

set BuildFolder=built

set ImguiPath="external\imgui"
set CommonCompilerFlags=/MTd /nologo /fp:fast /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /Iimgui /Fo%BuildFolder%\
set CommonLinkerFlags=/incremental:no /opt:ref /subsystem:console d3d11.lib d3dcompiler.lib

set AdditionalSourceFiles=imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_demo.cpp imgui/imgui_tables.cpp imgui/imgui_widgets.cpp source/imgui_impl_win32.cpp source/imgui_impl_dx11.cpp

if not exist %BuildFolder%\ mkdir %BuildFolder%

cl.exe %CommonCompilerFlags% source/win32_main.cpp %AdditionalSourceFiles% /Fe%BuildFolder%/%ProjectExe% /link %CommonLinkerFlags%
