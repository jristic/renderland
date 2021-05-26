# RenderLand

RenderLand is a Windows program for dynamically specifying D3D11/HLSL routines via a description format (RLF) that is live-editable. 

RenderLand's goal is to allow one to experiment and prototype graphics techniques without having to write all the associated code to glue the shaders together. Where possible it will ingest normal HLSL shader files with the instructions specified in the RLF file on how to glue it all together. 

## Status
RenderLand is an early work in progress. Currently only the most basic draws are supported, and RLF doesn't allow specifying all the possibilities one would expect. 

## Usage
See the samples in `samples/`.

## Prerequisites for building
1. A Visual Studio 2017 installation (Community edition will work).

## Building
1. If necessary, edit shell.bat to correctly point to vcvarsall.bat for your VS installation.
2. You will need to run shell.bat in your environment to set up for compiling with VS tools.
3. Run 'build'. This builds a debug configuration by default. Run 'build release' for an optimized build. 
