# RenderLand

RenderLand is a Windows program for specifying D3D11/HLSL routines via a description format (RLF) that is live-editable. 

RenderLand's goal is to allow one to experiment and prototype graphics techniques without having to write all the associated code to glue the shaders together. It ingests normal HLSL shader files with the instructions specified in the RLF file on how to use them.

## Status
RenderLand is an early work in progress. Currently RLF doesn't allow specifying all the possibilities one would expect from D3D, including but not limited to:
* Dynamic texture sizing.
* Viewport and Blend state. 
* Multiple render targets.
* Multiple vertex buffers.
* Instancing for draws.
* Indirect draws.
* Append/consume buffers.
* MSAA.  

Furthermore, the following features are not yet implemented/complete:
* Robust dynamic expressions for filling constant buffers. 
* RLF quality of life - includes, templating, etc. 
* Application UI. 
* Comprehensive samples which demonstrate every aspect of RLF. 

See the plan.txt for more detail. 

## Usage
See the samples in `samples/`.

## Prerequisites for building
1. A Visual Studio 2017 installation (Community edition will work).

## Building
1. If necessary, edit shell.bat to correctly point to vcvarsall.bat for your VS installation.
2. You will need to run shell.bat in your environment to set up for compiling with VS tools.
3. Run 'build'. This builds a debug configuration by default. Run 'build release' for an optimized build. 
