# MagElementTestWindows

## Overview

C++ test program to run in a Windows console, to connect to a running instance of MagElement, to receive streaming data, and to display summary information in the console.  The code for this program includes descriptions of the data types streamed by MagElement.

#### Version history

Version 1.0.0: Basic information and build instructions, March 21, 2025.
Version 1.0.1: Minor changes to complete Window build.
Version 1.2.0: Test UDP streaming.

## Prerequisites

- Code in this repository
- Copy of Boost 1.83.0 or later in the file system accessible to your build tools.  This program uses header-only Boost files in the asio and supporting libraries; no scripts or compiled libraries should be needed.
- Windows build tools: This program has been built and tested with Microsoft Visual Studio 2022.  The sources are not complicated, and should be build-able with other Windows-target build tools.

## Building the test program

Visual Studio 2022

This repository includes a VS solution file. Open it; try to build; change any settings local to your build environment, including locations of Boost asio header files.

