@echo off

call project.bat

set BuildFolder=build

set VulkanIncludePath="%VK_SDK_PATH%\Include"
set VulkanLibPath="%VK_SDK_PATH%\Lib"
set SdlIncludePath="SDL\include"
set SdlLibPath="SDL\lib\x64"
set CommonCompilerFlags=/MTd /nologo /fp:fast /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /FC /Z7 /D_CRT_SECURE_NO_WARNINGS /Iimgui /I%VulkanIncludePath% /I%SdlIncludePath% /Fo%BuildFolder%\
set CommonLinkerFlags=/incremental:no /opt:ref /subsystem:console /libpath:%VulkanLibPath% /libpath:%SdlLibPath% vulkan-1.lib sdl2.lib

set AdditionalSourceFiles=imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_demo.cpp imgui/imgui_widgets.cpp source/imgui_impl_sdl.cpp source/imgui_impl_vulkan.cpp

if not exist %BuildFolder%\ mkdir %BuildFolder%

cl.exe %CommonCompilerFlags% source/win32_main.cpp %AdditionalSourceFiles% /Fe%ProjectExe% /link %CommonLinkerFlags%
