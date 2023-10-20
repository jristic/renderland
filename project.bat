set ProjectName=renderland
set ProjectPath=%~dp0
set ProjectExe=%ProjectName%.exe

set GFXAPI=D3D11

set ProjectPlatformDefines=/D%GFXAPI%
set ProjectPlatformLinkLibs=%GFXAPI%.lib
