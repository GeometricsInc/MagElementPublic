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

/* See LICENSE file for additional licenses */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include "MagElementData.hpp"
#include "licensetext.h"

using namespace std; // For strlen.
using namespace boost::asio;

/* Use the Boost asio libraries for network functions */
using boost::asio::ip::tcp;

#define EXAMPLE_VERSION "1.2.0"
#define max_length 1296

/*******************************************************************
/* Define some search sequences. This program finds packets 
   by reading bytes until it finds a sequence that corresponds to the 
   sequence of { [TypeIdentifier],[TypeSize] } for one of the
   candidate packets. This approach can produce a false positive - 
   one of those 8-byte patterns could appear in normal data, 
   but it's very unlikely that the
   pattern will repeat itself numerous times, and once this program
   has locked on to correct packet sequence it should stay locked on.
*/

/* Hard-coded check to verify that compiler packing is correct */
static_assert ((sizeof(StreamerPacket) == 1296),"Not expected size");
    
/* Define the search sequence for the 1000Hz streamer packets. */
uint32_t sequenceStart[2] {GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS,
    sizeof (StreamerPacket) };
uint8_t *startPtr = (uint8_t*)sequenceStart;
const    uint32_t     SEQUENCE_START_LENGTH = 8;

/* Define the search sequence for the decimated magnetometer packets */
uint32_t decimatedStart[2] {GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT,
    sizeof (IndexedMagElementDecimatedMagPacketWithHeader) };
const    uint32_t     DECIMATED_START_LENGTH = 8;

/* Define the search sequence for the 1Hz status packets, aka the "heartbeat" packets. */
uint32_t heartbeatStart[2] {GM_MAG_ELEMENT_HEARTBEAT_FORMAT,sizeof (GmMagElementStatusPacket) };
const    uint32_t     HEARTBEAT_START_LENGTH = 8;

/* A few convenient values */
#define PACKET_LENGTH           (sizeof(StreamerPacket))
#define HEARTBEAT_LENGTH        (sizeof (GmMagElementStatusPacket))
#define DECIMATED_RECORD_LENGTH (sizeof (IndexedMagElementDecimatedMagPacketWithHeader))
/******************************************************************


/* \brief Find the location of a sequence of bytes (the needle) 
     within a larger sequence (the haystack).
   \return  - 1 :     - Not found.
   \return  0,1,2...  - Needle found at that index in the haystack.
*/
int32_t findStartOffset (const uint8_t *haystack,
                         const uint32_t haystackLength,
                         const uint8_t *needle,
                         const uint32_t needleLength,
                         const uint32_t dataToScan )
{
  uint32_t dataScanned = 0;
  uint32_t index = 0;
  uint32_t needleIndex = 0;
  uint32_t haystackIndex = 0;
  while (haystackIndex < haystackLength)
    {
      if (needle[needleIndex] == haystack[haystackIndex])
        {
          needleIndex++;
          haystackIndex++;
          dataScanned++;
          if (dataScanned > dataToScan)
            {
              return -1;
            }
          if (needleIndex == needleLength)
            {
              return (haystackIndex - needleLength);
            }
        }
      else
        {
          needleIndex = 0;
          haystackIndex++;
        }
    }
  return -1;
}


/* \brief Connect to the instrument, then stream data. 
    This function does not retry, or reconnect after a connection 
    failure, or exit gracefully. This is not a production program. */
int RunTcpClient (int argc, char* argv[])
{
  try {
    /* Basic asio setup */
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =

    /* Set up with command line arguments*/
    resolver.resolve(tcp::v4(), argv[1], argv[2]);

    /* Or the could be hard-coded */
    //resolver.resolve(tcp::v4(), "192.168.0.131", "3000");
    tcp::socket s(io_context);

    /* Attempt the connection. If the connection is made, the
       MagElement should start streaming data into the connection. */
    boost::asio::connect(s, endpoints);
    
    uint32_t counter = 0;
    
    while (true)
      {
        while (true)
          {
            /* Buffer into which data will be read.  It is as long or longer than
               the longest expected packet; this means that if the instrument and the
               network are functioning correctly, it should always include the header
               for at least one valid record, and once the program has locked on
               to the packet sequence this buffer can hold an entire packet of any
               valid type. */
            uint8_t receiveBuffer[max_length];
            
            bool found = false;
            found = false;

            /* Look for heartbeat records; we should always get one within about 1 second,
               if the MagElement is functioning correctly */
            int leftover = -1;

            /* Read a heartbeat's worth of data into the the buffer. Eventually one will be 
               found. */
            size_t receiveBufferLength = boost::asio::read(s, boost::asio::buffer(receiveBuffer, HEARTBEAT_LENGTH));

            /* Does the buffer include a heartbeat header? */
            int test = findStartOffset(receiveBuffer, receiveBufferLength, (uint8_t *) heartbeatStart, HEARTBEAT_START_LENGTH,
                                       HEARTBEAT_LENGTH);
          
            /* If found .... */
            if (test >= 0) {
              /* This program is now locked on. */
              found = true;

              /* Retrieve the rest of this heartbeat. Discard it; don't process. */
              leftover = HEARTBEAT_LENGTH - (receiveBufferLength - test);
              while (leftover > 0) {
                receiveBufferLength = boost::asio::read(s, boost::asio::buffer(receiveBuffer, leftover));
                leftover -= receiveBufferLength;
              }
            } else {
              leftover = -1;
            }

            /* If locked on, exit this loop... */
            if (found)
              {
                printf ("Found heartbeat.\n");
                break;
              }
            /* Else continue looking ... */
          }
      
        /* Locked on; read records */

        while (true) {
          counter++;
          uint8_t receiveBuffer[2000];

          /* Read an 8-bite header. All records from MagElement include an 8-byte header.*/
          typedef uint32_t IntAndSizeHeader[2];
          IntAndSizeHeader* testHeader;
          bool recognizedRecord = false;
          size_t bytesReceived = boost::asio::read(s, boost::asio::buffer((uint8_t*)receiveBuffer, sizeof(IntAndSizeHeader)));

          /* Found? */
          if (bytesReceived == 8)
            {
              testHeader = (IntAndSizeHeader*)&receiveBuffer;
              switch ((*testHeader)[0])
                {
                  /* Is the type known? */
                case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
                case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
                case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
                  recognizedRecord = true;

                  /* Read the remaining part  of the packet */
                  bytesReceived = boost::asio::read(s, boost::asio::buffer(receiveBuffer + 8, (*testHeader)[1]-8));
                  switch ((*testHeader)[0])
                    {

                      /* 1000Hz block packet */
                    case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
                      {
                        StreamerPacket *streamerPacket = (StreamerPacket *) &receiveBuffer;
          
                        char *name = (char *) &receiveBuffer;
                        cout << counter << ":" << name << ":"
                             << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag1data) << ":"
                             << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag2data) << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc0 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc1 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc2 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc3 << "\n";
                      }
                      break;

                      /* Decimated magnetometer packet */
                    case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
                      {
                        IndexedMagElementDecimatedMagPacketWithHeader *decimatedPacket = (IndexedMagElementDecimatedMagPacketWithHeader *) &receiveBuffer;
          
                        cout << counter << ":"
                          //                         << ":" << GET_FID_COUNT (decimatedPacket->mIndexedPacket.mPacket.frameid)
                             << ":" << decimatedPacket->mIndexedPacket.mPacket.mMagData
                             << "\n";
                      }
                      break;

                      /* Heartbeat/status packet */
                    case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
                      {
                        GmMagElementStatusPacket *statusPacket = (GmMagElementStatusPacket*)&receiveBuffer;
                        cout << "Status: " << statusPacket->mIndex
                             << ":" << statusPacket->mCounterAtFirstPps
                             << ":" << statusPacket->mCounterAtLastPps
                             << ":" << statusPacket->mMfamStatus[0]
                             << ":" << statusPacket->mMfamStatus[1]
                             << ":" << statusPacket->mMfamStatus[2]
                             << ":" << statusPacket->mMfamStatus[3]
                             << "\n";
                      }
                      break;
                    }
                default:
                  break;
                }
              /* If the program isn't locked onto a recognized record type,
                 then go back to the startup search function */
              if (!recognizedRecord)
                {
                  break;
                }
            }
        }
      }
  }
  catch (std::exception &e) {
    /* Simplest possible error handling: This program (which is not
       a production program, will exit with some raw error information */
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}


int RunUdpClient (int argc, char* argv[])
{
  ip::udp::endpoint remote_endpoint;

  try {
    /* Basic asio setup */
    boost::asio::io_context io_context;
    ip::udp::endpoint ep(ip::udp::v4(),atoi(argv[1]));
    ip::udp::socket sock(io_context,ep);
    
    uint32_t counter = 0;

    uint8_t receiveBuffer[max_length];

    
    while (true)
      {
        while (true)
          {
            /* Buffer into which data will be read.  It is as long or longer than
               the longest expected packet; this means that if the instrument and the
               network are functioning correctly, it should always include the header
               for at least one valid record, and once the program has locked on
               to the packet sequence this buffer can hold an entire packet of any
               valid type. */
            
            bool found = false;
            found = false;

            /* Look for heartbeat records; we should always get one within about 1 second,
               if the MagElement is functioning correctly */
            int leftover = -1;

            /* Read a heartbeat's worth of data into the the buffer. Eventually one will be 
               found. */
            size_t bytesReceived = sock.receive_from(boost::asio::buffer(receiveBuffer, max_length),
                                                           remote_endpoint);
            cout << bytesReceived << std::endl;
            //size_t bytesReceived = boost::asio::read(s, boost::asio::buffer(receiveBuffer, HEARTBEAT_LENGTH));

            /* Does the buffer include a heartbeat header? */
            int test = findStartOffset(receiveBuffer, bytesReceived, (uint8_t *) heartbeatStart, HEARTBEAT_START_LENGTH,
                                       HEARTBEAT_LENGTH);
          
            /* If found .... */
            if (test >= 0) {
              /* This program is now locked on. */
              found = true;
            }

            /* If locked on, exit this loop... */
            if (found)
              {
                printf ("Found heartbeat.\n");
                break;
              }
            /* Else continue looking ... */
          }
      
        /* Locked on; read records */

        while (true) {
          counter++;

          /* Read an 8-byte header. All records from MagElement include an 8-byte header.*/
          typedef uint32_t IntAndSizeHeader[2];
          IntAndSizeHeader* testHeader;
          bool recognizedRecord = false;
          size_t bytesReceived = sock.receive_from( boost::asio::buffer((uint8_t*)receiveBuffer, max_length),remote_endpoint);

          /* Found? */
          if (bytesReceived >  8)
            {
              testHeader = (IntAndSizeHeader*)&receiveBuffer;
              switch ((*testHeader)[0])
                {
                  /* Is the type known? */
                case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
                case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
                case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
                  recognizedRecord = true;

                  /* Read the remaining part  of the packet */
                  //              cout<<"Remaining: " << ((*testHeader)[1])-8 << std::endl;
                  //                  bytesReceived = sock.receive_from (boost::asio::buffer(receiveBuffer + 8, ((*testHeader)[1])-8),remote_endpoint);
                  //        cout << bytesReceived << std::endl;
                  switch ((*testHeader)[0])
                    {

                      /* 1000Hz block packet */
                    case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
                      {
                        StreamerPacket *streamerPacket = (StreamerPacket *) &receiveBuffer;
          
                        char *name = (char *) &receiveBuffer;
                        cout << counter << ":" << name << ":"
                             << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag1data) << ":"
                             << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag2data) << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc0 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc1 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc2 << ":"
                             << streamerPacket->mDataBlock[0].mAnalogs.adc3 << "\n";
                      }
                      break;

                      /* Decimated magnetometer packet */
                    case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
                      {
                        IndexedMagElementDecimatedMagPacketWithHeader *decimatedPacket = (IndexedMagElementDecimatedMagPacketWithHeader *) &receiveBuffer;
          
                        cout << counter << ":"
                          //                         << ":" << GET_FID_COUNT (decimatedPacket->mIndexedPacket.mPacket.frameid)
                             << ":" << decimatedPacket->mIndexedPacket.mPacket.mMagData
                             << "\n";
                      }
                      break;

                      /* Heartbeat/status packet */
                    case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
                      {
                        GmMagElementStatusPacket *statusPacket = (GmMagElementStatusPacket*)&receiveBuffer;
                        cout << "Status: " << statusPacket->mIndex
                             << ":" << statusPacket->mCounterAtFirstPps
                             << ":" << statusPacket->mCounterAtLastPps
                             << ":" << statusPacket->mMfamStatus[0]
                             << ":" << statusPacket->mMfamStatus[1]
                             << ":" << statusPacket->mMfamStatus[2]
                             << ":" << statusPacket->mMfamStatus[3]
                             << "\n";
                      }
                      break;
                    }
                  break;
                default:
                  cout << "Unrecognized.\n";
                  break;
                }
              /* If the program isn't locked onto a recognized record type,
                 then go back to the startup search function */
              if (!recognizedRecord)
                {
                  break;
                }
            }
        }
      }
  }
  catch (std::exception &e) {
    /* Simplest possible error handling: This program (which is not
       a production program, will exit with some raw error information */
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}

#ifdef _WIN32
#define APPLICATION_NAME "MagElementTestWindows"
#else
#define APPLICATION_NAME "MagElementTestLinux"
#endif



int main(int argc, char* argv[])
{
  std::cout << "===========================================================\n";
  std::cout << APPLICATION_NAME << " version " << EXAMPLE_VERSION << std::endl;
  if ((argc == 2) && (strcmp (argv[1], "LICENSE") == 0))
    {
      std::cout << licenseText;
    }
  else
    {
      if (argc != 3) {
        std::cout <<
R"(
TCP streaming:
  MagElementTestLinux <IP Address> <port>
  For example:
     MagElementTestLinux 192.168.10.3 1000
       or
     MagElementTestWindows 192.168.10.3 1000

  UDP streaming:
    MagElementTestLinux <port> udp
    For example:
     MagElementTestLinux 2000 udp
       or
     MagElementTestWindows 2000 udp   
    

View license:
  MagElementTestLinux LICENSE | more
    or
  MagElementTestWindows LICENSE : more

In TCP mode, this program attempts to
connect to a MagElement, at the specified address, 
on the specified port. If the instrument is listening 
on that port, it accepts the connection and starts 
streaming data.

In UDP mode, this program listens on the specified
port for data from a MagElement that has been 
configured to send data to this computer on that port.
)";
  std::cout << "===========================================================\n";
        
        
        return 1;
      }

      if (strcmp(argv[2],"udp") == 0)
	{
	  return RunUdpClient (argc,argv);
	}
      else
	{
	  return RunTcpClient (argc,argv);
	}
    }
}

