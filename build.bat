@echo off

IF NOT EXIST build mkdir build

set CommonCompilerFlags=/MTd /nologo /fp:fast /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /FC /Z7
set CommonLinkerFlags=/incremental:no /opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

pushd build
cl.exe %CommonCompilerFlags% ../source/main.cpp /Ferenderland.exe /link %CommonLinkerFlags%
popd