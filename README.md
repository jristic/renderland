# RenderLand

RenderLand is a Windows program for specifying D3D/HLSL routines via a description format (RLF) that is live-editable. 

RenderLand's goal is to allow one to experiment and prototype graphics techniques without having to write all the associated code to glue the shaders together. It ingests normal HLSL shader files with the instructions specified in the RLF file on how to use them.

## Status
RenderLand is an early work in progress. Currently RLF doesn't allow specifying all the possibilities one would expect from D3D, including but not limited to:
* Texture types: Array, 3D, Cube
* Append/consume buffers.

Furthermore, the following features are not yet implemented/complete:
* Robust dynamic expressions for filling constant buffers. 
* RLF quality of life - includes, templating, etc. 
* Application UI. 
* Comprehensive samples which demonstrate every aspect of RLF. 

See the plan.txt for more detail. 

## Usage
See the samples in `samples/`.

## Backends
Currently both D3D11 and D3D12 backends exist. The D3D11 backend is currently recommended as it is better tested and handles errors more gracefully. The backend can be selected by changing the value of `GfxApi` in `project.bat` and recompiling.

## Prerequisites for building
1. A Visual Studio 2017 installation (Community edition will work).

## Building
1. If necessary, edit shell.bat to correctly point to vcvarsall.bat for your VS installation.
2. You will need to run shell.bat in your environment to set up for compiling with VS tools.
3. Run 'prebuild'. This builds a debug configuration by default. Run 'prebuild release' for an optimized build. This needs to match whether you build release or debug for the next step. The files included in this compilation unit are not generally changed so it can be skipped on future compiles if not changing external source files or global compilation parameters. 
4. Run 'build'. This builds a debug configuration by default. Run 'build release' for an optimized build. 
