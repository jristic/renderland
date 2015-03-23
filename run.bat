@echo off

call build.bat

pushd build
renderland.exe
popd