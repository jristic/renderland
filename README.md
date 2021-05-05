# RenderLand

RenderLand is a Windows program for dynamically specifying D3D11/HLSL routines via a description format (RLF) that is live-editable. 

Dep works by having you pass the command you want executed through it, which it creates for you while injecting a DLL into the created process that tracks all files used as input to and output from the process. If the outputs are already up-to-date given the current state of the inputs, Dep skips invoking the command. 

## Status
RenderLand is an early work in progress. Currently only the compute path is supported, and RLF doesn't allow specifying all the possibilities one would expect. 

## Usage
See the samples in `samples/`.

## Prerequisites for building
1. A Visual Studio 2017 installation (Community edition will work).

## Building
1. If necessary, edit shell.bat to correctly point to vcvarsall.bat for your VS installation.
2. You will need to run shell.bat in your environment to set up for compiling with VS tools.
3. Run 'build'. This builds a debug configuration by default. Run 'build release' for an optimized build. 
