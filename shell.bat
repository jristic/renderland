@echo off

REM
REM  To run this at startup, use this as your shortcut target:
REM  %windir%\system32\cmd.exe /k F:\dev\renderland\shell.bat
REM

set ProjectPath=F:\dev\renderland

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
cd /d %ProjectPath%