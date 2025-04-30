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

const char *sHelpText = R"(Usages:
   MagElementTestLinux [OPTION]....
   MagElementTestWindows [OPTION]....

Examples:
  MagElementTestLinux -protocol tcp -addr 192.168.10.3 -port 1000
  MagElementTestLinux -protocol tcp -addr 192.168.10.3 -port 1000
  MagElementTestWindows -protocol udp -port 2000 -file "savefile.bin"
  MagElementTestWindows -protocol file-check -file "savefile.bin" 			   
  MagElementTestLinux -LICENSE
  

Options:
-protocol      [tcp | udp | file-check ] : tcp and udp are communications protocols
                   to receive data from a MagElement.  file-check is a command
                   to check the validity of the data in a data file collected
                   via udp or tcp
                   No default value.
-addr          Ip address of the sending instrument, in NNN.NNN.NNN.NNN format
-port          Instrument port to which this test should connect; used for tcp only.
-file          Optionally, open this file and record all binary records to this file.
-verbose       Display info to console.  [ true | false ]. Default = true; 
                 in non-verbose mode the program may run silently, 
                 i.e. without any indication that it is running.
-LICENSE       Display the license for this software.
)";
