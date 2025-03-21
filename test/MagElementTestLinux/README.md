# MagElementTestLinux

Version history:

Version 1: Basic information and build instructions, March 21, 2025.

C++ test program to run in a Linux console, to connect to a running instance of MagElement, to receive streaming data, and to display summary information in the console.  The code for this program includes descriptions of the data types streamed by MagElement.

## Prerequisites

To use the CMake-based build system in this repository

- CMake version 3.27 or later

- G++ compiler supporting C++20.

- Boost 1.83.0 or later, pulled from here - https://www.boost.org/users/download/ - and extracted into a development directory.  This program uses the header-only version of Boost asio, and doesn't require running any Boost setup scripts or compiles.

- To use the CMake information in the project, familiarity with the tools and CMake build processes is required.  The build information is contained in CMakeLists.txt.  

  

## Building the test program

There exist two options:

1. Build using the CMake build information in the code directory
2. Build using any other make system or IDE: Visual code, CLion, etc.  The code is very simple - one header file and one code file - and can be built in any C++ IDE or make system.

## CMake builds

From a terminal in the main directory (the one containing CMakeLists.txt), follow these instructions:

`/* Make a build directory */
mkdir build
/* Move into build directory */
cd build
/* Generate the build system in current directory. This uses info in CMakeLists.txt */
cmake ..
/* Build the program */
make
/* After cleaning up any build issues (CMake, include directories defined in CMakeLists.txt, etc.),  execute the program. */`
`./MagElementTestLinux`