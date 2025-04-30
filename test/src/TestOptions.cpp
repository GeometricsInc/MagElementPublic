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
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "TestOptions.hpp"

void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

// trim from end (in place)
void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), s.end());
}


bool removeQuotes (std::string &s)
{
  if (s.front() == '"')
    {
      s = s.substr (1,s.length() - 1);
      if (s.length () == 0)
	{
	  return false;
	}
      if (s.back() == '"')
	{
	  s = s.substr (0,s.length() - 1);
	}
    }
  if (s.back() == '"')
    {
      return false;
    }
  return true;
}

bool isValidIpAddress (char *candidate)
{
  std::string test{candidate};
  ltrim(test);
  rtrim(test);

  std::vector<std::string> output;

  std::string::size_type prev_pos = 0, pos = 0;

  while((pos = test.find('.',pos)) != std::string::npos)
    {
      std::string substring( test.substr(prev_pos, pos-prev_pos) );
      output.push_back(substring);
      prev_pos = ++pos;
    }
  std::string last{test.substr(prev_pos, pos-prev_pos)}; // Last word
  if (last.size() > 0)
    {
      output.push_back(last);
    }

  if (output.size() != 4)
    {
      return false;
    }

  for (auto nextString = output.begin(); nextString != output.end(); nextString++)
    {
      try
	{
	  int rawLength = (*nextString).size();
	  ltrim(*nextString);
	  rtrim(*nextString);
	  if (rawLength != (*nextString).size())
	    {
	      return false;
	    }
	  std::size_t processed = 0;
	  int testValue = std::stoi((*nextString),&processed, 10);
	  if (processed != (*nextString).size())
	    {
	      return false;
	    }
	  if ((testValue < 0)||(testValue > 255))
	    {
	      return false;
	    }
	}
      catch (std::exception)
	{
	  return false;
	}
    }
  return true;
}

MagElementTestOptions::MagElementTestOptions (int countArgs, char *argv[])
{
  for (int index = 1; index < countArgs; index++)
    {
      std::string nextArg { argv[index] };
      if (!removeQuotes (nextArg))
	{
	  mValid = false;
	  std::cerr << "\n\nError:: Format of parameter " << argv[index] << " is invalid.\n\n";
	  return;
	}

      if (nextArg == "-proto")
	{
	  if (countArgs <= index)
	    {
	      mValid = false;
	      std::cerr << "\n\nError: -proto needs to be followed by udp or tcp\n\n";
	      return;
	    }
	  index++;
	  nextArg = std::string { argv[index]};
	  if (!removeQuotes (nextArg))
	    {
	      std::cerr << "\n\nError: Format of parameter " << argv[index] << " is invalid.\n\n";
	      mValid = false;
	      return;
	    }
	  if (nextArg == "udp")
	    {
	      mAcceptUdp = true;
	    }
	  else if  (nextArg == "tcp")
	    {
	      mAcceptTcp = true;
	    }
	}
      else if (nextArg == "-LICENSE")
	{
	  mValid = false;
	  return;
	}
      else if (nextArg ==  "-addr")
	{
	  index++;
	  if (countArgs <= index)
	    {
	      mValid = false;
	      std::cerr << "\n\nError: -addr needs to be followed by a valid IP address\n\n";
	      return;
	    }
	  nextArg = std::string { argv[index]};
	  if (!removeQuotes (nextArg))
	    {
	      std::cerr << "\n\nError: -addr needs to be followed by a valid IP address\n\n";
	      mValid = false;
	      return;
	    }
	  if (!isValidIpAddress (nextArg.data()))
	    {
	      std::cerr << "\n\nError: IP address is not valid.\n\n";
	      mValid = false;
	      return;
	    }
	  mRemote = nextArg;
	  mRemoteIsValid = true;
	}
      else if (nextArg == "-port")
	{
	  try {
	    uint32_t portNumber = std::stoul(nextArg);
	    if ((portNumber == 0)||(portNumber > 65535))
	      {
		std::cerr << "\n\nError: Port number is invalid\n\n";
		mValid = false;
		return;
	      }
	  }
	  catch (std::exception &e)
	    {
	      mValid = false;
	      std::cerr << "Invalid port number.\n\n";
	      return;
	    }
	  mRemotePort = nextArg;
	}
      else if (nextArg == "-file")
	{
	  index++;
	  if (countArgs <= index)
	    {
	      mValid = false;
	      std::cerr << "\n\nError: -file needs to be followed by a valid file name\n\n";
	      return;
	    }
	  nextArg = std::string { argv[index]};
	  if (!removeQuotes (nextArg))
	    {
	      std::cerr << "\n\nError: -file needs to be followed by a valid file name\n\n";
	      mValid = false;
	      return;
	    }
	  if (std::filesystem::exists(nextArg))
	    {
	      std::cerr << "\n\nError: Output file already exists.\n\n";
	      mValid = false;
	      return;
	    }
	  
	  mFileNameToSave = nextArg;
	  mFileIsValid = true;
	}
      else if (nextArg == "-verbose")
	{
	  index++;
	  if (countArgs <= index)
	    {
	      mValid = false;
	      std::cerr << "\n\nError: -verbose must be followed by true or false\n\n";
	      return;
	    }
	  nextArg = std::string { argv[index]};
	  if (!removeQuotes (nextArg))
	    {
	      std::cerr << "\n\nError: -verbose must to be followed by a valid value\n\n";
	      mValid = false;
	      return;
	    }
	  if (nextArg == "true")
	    {
	      mVerboseMode = true;
	    }
	  else if (nextArg == "false")
	    {
	      mVerboseMode = false;
	    }
	  else
	    {
	      std::cerr << "\n\nError: -verbose must to be followed by a valid value\n\n";
	      mValid = false;
	      return;
	    }
	}
      else
	{
	  std::cerr << "\n\nError: Parameter " << argv[index] << " is invalid.\n\n";
	  mValid = false;
	  return;
	};
      
    };
  if ((!(mAcceptUdp || mAcceptTcp))||(mAcceptUdp && mAcceptTcp))
    {
      std::cerr << "\n\nError: Either tcp or udp must be selected.\n\n";
      mValid = false;
      return;
    }
  if (!mRemoteIsValid)
    {
      std::cerr << "\n\nError: Not a valid remote IP address.\n\n";
      mValid = false;
      return;
    }
  mValid = true;
}

