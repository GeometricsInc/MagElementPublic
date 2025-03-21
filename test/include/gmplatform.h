/*******************************************************************************
Copyright 2025 Geometrics, Inc.

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the “Software”), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
******************************************************************************/

/* Definitions of alignment and packing compiler programs, keyed on the
   platform for which these programs are being built. */

#ifndef GM_PLATFORM_HPP
#define GM_PLATFORM_HPP

#ifdef _WIN32
typedef long long ssize_t; // same as SSIZE_T from windows stdint.h
#define PACKED_SPEC
#else
#define PACKED_SPEC __attribute__ ((__packed__))
#endif

#ifdef _WIN32
#define ALIGN_8_SPEC
#else
#define ALIGN_8_SPEC __attribute__ ((aligned (8)))
#endif

#ifdef _WIN32
#define ALIGN_4_SPEC
#else
#define ALIGN_4_SPEC __attribute__ ((aligned (4)))
#endif

#ifdef _WIN32
#define ALIGN_1_SPEC
#else
#define ALIGN_1_SPEC __attribute__ ((aligned (1)))
#endif

#ifdef _WIN32
#define PACKED_PRAGMA __pragma(pack(2))
#else
#define PACKED_PRAGMA
#endif

#endif
