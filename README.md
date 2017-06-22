# amd-tootle [![Build status](https://ci.appveyor.com/api/projects/status/t9kao41or7eqp1dg?svg=true)](https://ci.appveyor.com/project/bpurnomo/amd-tootle) [![Build Status](https://travis-ci.org/GPUOpen-Tools/amd-tootle.svg?branch=master)](https://travis-ci.org/GPUOpen-Tools/amd-tootle) [![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
The amd-tootle repository includes the source code of the AMD Tootle library and an application command line tool.  

AMD Tootle (Triangle Order Optimization Tool) is a 3D triangle mesh optimization library that improves on existing mesh preprocessing techniques. By using AMD Tootle, developers can optimize their models for pixel overdraw as well as vertex cache performance. This can provide significant performance improvements in pixel limited situations, with no penalty in vertex-limited scenarios, and no runtime cost.

# AMD Tootle Features
1. **Vertex cache optimization**: Triangles are re-ordered to optimize for the post-transform vertex cache in modern GPUs. This will yield significant performance improvements in vertex-tranform limited scenes.
2. **Overdraw optimization**: To reduce the pixel cost of rendering a mesh, the AMD Tootle library further re-orders the triangles in the mesh to reduce pixel overdraw. Significant reductions in pixel overdraw (2x or higher) can be achieved. This can yield significant performance improvements in pixel-limited scenes, and incurs no penalty in vertex-limited scenarios.
3. **Vertex prefetch cache optimization**: Triangle indices are re-indexed in the order of their occurrence in the triangle list. The vertex buffer is re-ordered to match these new indices. Thus, vertices are accessed close to each other in memory. This optimization exploits the input vertex cache because vertices are typically fetched in a cacheline (that may contains more than one vertex data).

AMD Tootle supports Microsoft Windows and Linux platform.

# Package Contents
The amd-tootle package has the following directory structure.
- *bin*: the location for the output AMD Tootle executable and DLL files
- *Build*
  - *VS2012*: contains the AMD Tootle solution and project files for Microsoft Visual Studio 2012
  - *VS2013*: contains the AMD Tootle solution and project files for Microsoft Visual Studio 2013
  - *VS2015*: contains the AMD Tootle solution and project files for Microsoft Visual Studio 2015
  - *VS2015 - DX 11.1*: contains the AMD Tootle solution and project files for Microsoft Visual Studio 2015 without DirectX SDK June 2010 dependency (requires DirectXMesh project)
  - *DirectX.props*: the Visual Studio property file that points to the location of Microsoft DirectX SDK in the system
    * Update this file to point to the location of the Microsoft DirectX SDK in your system (the current support is for Microsoft DirectX SDK June 2010)
- *docs*: contains tootle papers and presentations
- *lib*: the location for the output AMD Tootle static library and import files
- *meshes*: contains several sample meshes
- *src*
  - *TootleLib*: contains the source code of the AMD Tootle library that can be linked to your mesh processing pipeline. There are multiple different build targets supported.
    - *include/tootlelib.h*: the AMD Tootle library header file
  - *TootleSample*: contains the source code of a sample application that reads a single material triangle mesh file *.obj* and exposes the functionality of the AMD Tootle library using a command line interface.

# Build and Run Steps
1. Set up Microsoft DirectX SDK dependency (the current support is for Microsoft DirectX SDK June 2010)
  * [Install DirectX SDK June 2010](https://www.microsoft.com/en-us/download/details.aspx?id=6812). **To skip this step for Windows 8 and later versions, use VS2015 - DX_11.1 solution file.** 
2. Clone the amd-tootle repository
  * `git clone https://github.com/GPUOpen-Tools/amd-tootle.git`
3. Build the AMD Tootle library and command line tool
  * On Windows
    * Open *Build/VS2015/Tootle.sln* if working with Microsoft Visual Studio 2015. 
	  * If you want to build without DirectX SDK June 2010, then
	    * Run `python Scripts/FetchDependencies.py`
	    * Open *Build/VS2015 - DX11_1/Tootle.sln* instead
	  * Select the appropriate build target for AMD Tootle within Microsoft Visual Studio
	  * Build all the projects
	  * The output files will be generated in the *bin* and *lib* folder
	  * A *runTest.bat* batch file is provided in the *bin* folder as an example for running the Tootle command line tool (*TootleSample.exe*)
	    * On a Windows console window, you can run *TootleSample.exe* without any command line arguments to print the instruction for running the tool
  * On Linux
    * Build the AMD Tootle library
      * `cd src/TootleLib`
	    * `make`
	  * Build the AMD Tootle command line tool
	    * `cd ../TootleSample`
	    * `make`
	  * Run the AMD Tootle command line tool
	    * `cd ../../bin`
	  * `./TootleSample -a 5 ../meshes/bolt.obj > out.obj`
	    * You can run `./TootleSample` without any command line arguments to print the instruction for running the tool

# References
1. Nehab, D. Barczak, J. Sander, P.V. 2006. Triangle Order Optimization for graphics hardware computation culling. In Proceedings of the ACM SIGGRAPH Symposium on Interactive 3D Graphics and Games, pages 207-211
2. Sander, P.V., Nehab, D. Barczak, J. 2007. Fast Triangle Reordering for Vertex Locality and Reduced Overdraw. ACM Transactions of Graphics (Proc. SIGGRAPH), 26(3), August 2007.
