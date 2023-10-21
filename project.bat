set ProjectName=renderland
set ProjectPath=%~dp0
set ProjectExe=%ProjectName%.exe

set GfxApi=D3D11

set ProjectPlatformDefines=/D%GfxApi%
set ProjectPlatformLinkLibs=%GfxApi%.lib
