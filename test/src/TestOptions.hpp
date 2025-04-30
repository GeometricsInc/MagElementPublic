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
#ifndef TEST_OPTIONS_HPP
#define TEST_OPTIONS_HPP

#define MAG_ELEMENT_MAX_PATH_LENGTH   255
#define MAG_ELEMENT_IP_ADDR_MAX_LENGTH 15

struct MagElementTestOptions
{
  MagElementTestOptions ( int countArgs, char *argv[] );
    
  bool         mSaveToFile = false;
  std::string  mFileNameToSave;
  bool         mFileIsValid = false;
  bool         mAcceptTcp = false;
  bool         mAcceptUdp = false;
  bool         mRunFileCheck = false;
  std::string  mRemote;
  bool         mRemoteIsValid = false;
  bool         mLicenseRequest = false;
  std::string  mRemotePort;
  bool         mRemotePortIsValid = false;
  bool         mValid = false;
  bool         mVerboseMode = true;
};

#endif
