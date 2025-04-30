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
#define EXAMPLE_VERSION "1.2.3"


#include <cstdlib>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include "MagElementData.hpp"
#include "licensetext.h"
#include "helptext.h"
#include "TestOptions.hpp"
#include <thread>

using namespace std; // For strlen.
using namespace boost::asio;

/* Use the Boost asio libraries for network functions */
using boost::asio::ip::tcp;

#define max_length 1296

/* Functions watch this variable in order to produce an
   orderly shutdown. */
static bool sShutDown = false;

/* Scan for keyboard command to quit. This function, running in a 
   separate thread, can also be used as a template for other means to
   control the instrument and its output. */

void WaitForCommand ()
{
  while (true)
    {
      cout << ":";
      char nextChar = getchar ();

      cout << "Found char: " << nextChar << "\n";

      /* Pressing 'q' causes an orderly shutdown */
      if (nextChar == 'q')
	{
          cout << "Shutting down.\n";
	  sShutDown = true;

	  /* Three seconds is long enough for the other threads to 
	     manage an orderly shutdown.  If they don't manage it, then be 
	     disorderly. */
          std::this_thread::sleep_for(std::chrono::seconds(3));
          cout << "Exiting.\n";
	  exit (0);
	}
    }
}


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
/******************************************************************/


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


/* Output packet to console or file. This is the place to add custom handling for this data type */
void HandleRawDataBlock (StreamerPacket *streamerPacket,
			 const int32_t counter,
			 MagElementTestOptions &options,
			 FILE *outputFile)
{
  if (options.mVerboseMode)
    {
      char *name = (char*)streamerPacket;
      /* Print only the first record in the block in order to verify validity. Production programs
	 will need to iterate the records or save the block. */
      cout << counter << ":" << name << ":"
	   << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag1data) << ":"
	   << MAG_DATA_AS_FLOAT(streamerPacket->mDataBlock[0].mMagData.mag2data) << ":"
	   << streamerPacket->mDataBlock[0].mAnalogs.adc0 << ":"
	   << streamerPacket->mDataBlock[0].mAnalogs.adc1 << ":"
	   << streamerPacket->mDataBlock[0].mAnalogs.adc2 << ":"
	   << streamerPacket->mDataBlock[0].mAnalogs.adc3 << "\n";
    }
  if (outputFile != nullptr)
    {
      size_t written = fwrite (streamerPacket,1,sizeof(StreamerPacket),outputFile);
      if (written != sizeof (StreamerPacket))
	{
	  if (options.mVerboseMode)
	    {
	      cerr << "Error: Data block not written.\n\n";
	    }
	}
    }
}


/* Output decimated packet to console or file. This is the place to add custom handling for this data type */
void HandleDecimatedPacket (IndexedMagElementDecimatedMagPacketWithHeader *decimatedPacket,
			    const int32_t counter,
			    MagElementTestOptions &options,
			    FILE *outputFile)
{
  if (options.mVerboseMode)
    {
      cout << counter << ":"
	   << ":" << decimatedPacket->mIndexedPacket.mPacket.mMagData
	   << "\n";
    }
  if (outputFile != nullptr)
    {
      size_t written = fwrite (decimatedPacket,1,sizeof(IndexedMagElementDecimatedMagPacketWithHeader),outputFile);
      if (written != sizeof ( IndexedMagElementDecimatedMagPacketWithHeader))
	{
	  if (options.mVerboseMode)
	    {
	      cerr << "Error: Decimated data packet not written.\n\n";
	    }
	}
    }
}

/* Output 1Hz status packet to console or file. This is the place to add custom handling for this data type */
void HandleStatusPacket (GmMagElementStatusPacket *statusPacket,
                         const int32_t counter,
			 MagElementTestOptions &options,
			 FILE *outputFile)
{
  if (options.mVerboseMode)
    {
      cout << "Status: " << statusPacket->mIndex
	   << ":" << statusPacket->mCounterAtFirstPps
	   << ":" << statusPacket->mCounterAtLastPps
	   << ":" << statusPacket->mMfamStatus[0]
	   << ":" << statusPacket->mMfamStatus[1]
	   << ":" << statusPacket->mMfamStatus[2]
	   << ":" << statusPacket->mMfamStatus[3]
	   << "\n";
    }
  if (outputFile != nullptr)
    {
      size_t written = fwrite (statusPacket,1,sizeof( GmMagElementStatusPacket),outputFile);
      if (written != sizeof ( GmMagElementStatusPacket))
	{
	  if (options.mVerboseMode)
	    {
	      cerr << "Error: Status packet not written.\n\n";
	    }
	}
    }
}

/* \brief Connect to the instrument, then stream data. 
   This function does not retry, or reconnect after a connection 
   failure, or exit gracefully. This is not a production program. */
int RunTcpClient (MagElementTestOptions &options, FILE *outputFile)
{
  try {
    /* Basic asio setup */
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =

      /* Set up with command line arguments*/
      resolver.resolve(tcp::v4(), options.mRemote, options.mRemotePort);

    /* Attempt the connection. If the connection is made, the
       MagElement should start streaming data into the connection. */
    tcp::socket s(io_context);
    boost::asio::connect(s, endpoints);
    
    uint32_t counter = 0;
    
    while (true)
      {
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return 0;
	    }
	while (true)
	  {
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return 0;
	    }
	    /* Buffer into which data will be read.  It is as long or longer than
	       the longest expected packet; this means that if the instrument and the
	       network are functioning correctly, it should always include the header
	       for at least one valid record, and once the program has locked on
	       to the packet sequence this buffer can hold an entire packet of any
	       valid type. */
	    uint8_t reply[max_length];
            
	    bool found = false;
	    found = false;

	    /* Look for heartbeat records; we should always get one within about 1 second,
	       if the MagElement is functioning correctly */
	    int leftover = -1;

	    /* Read a heartbeat's worth of data into the the buffer. Eventually one will be 
	       found. */
	    size_t replyLength = boost::asio::read(s, boost::asio::buffer(reply, HEARTBEAT_LENGTH));

	    /* Does the buffer include a heartbeat header? */
	    int test = findStartOffset(reply, replyLength, (uint8_t *) heartbeatStart, HEARTBEAT_START_LENGTH,
				       HEARTBEAT_LENGTH);
          
	    /* If found .... */
	    if (test >= 0) {
	      /* This program is now locked on. */
	      found = true;

	      /* Retrieve the rest of this heartbeat. Discard it; don't process. */
	      leftover = HEARTBEAT_LENGTH - (replyLength - test);
	      while (leftover > 0) {
		replyLength = boost::asio::read(s, boost::asio::buffer(reply, leftover));
		leftover -= replyLength;
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
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return (0);
	    }
	  counter++;
	  uint8_t reply[2000];

	  /* Read an 8-bite header. All records from MagElement include an 8-byte header.*/
	  typedef uint32_t IntAndSizeHeader[2];
	  IntAndSizeHeader* testHeader;
	  bool recognizedRecord = false;
	  size_t replyLength = boost::asio::read(s, boost::asio::buffer((uint8_t*)reply, sizeof(IntAndSizeHeader)));

	  /* Found? */
	  if (replyLength == 8)
	    {
	      testHeader = (IntAndSizeHeader*)&reply;
	      switch ((*testHeader)[0])
		{
		  /* Is the type known? */
		case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
		case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
		case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
		  recognizedRecord = true;

		  /* Read the remaining part  of the packet */
		  replyLength = boost::asio::read(s, boost::asio::buffer(reply + 8, (*testHeader)[1]-8));
		  switch ((*testHeader)[0])
		    {
		      /* 1000Hz block packet */
		    case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
		      {
			StreamerPacket *streamerPacket = (StreamerPacket *) &reply;
			HandleRawDataBlock (streamerPacket, counter, options, outputFile);
		      }
		      break;

		      /* Decimated magnetometer packet */
		    case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
		      {
			IndexedMagElementDecimatedMagPacketWithHeader *decimatedPacket = (IndexedMagElementDecimatedMagPacketWithHeader *) &reply;
			HandleDecimatedPacket (decimatedPacket, counter, options, outputFile);
		      }
		      break;

		      /* Heartbeat/status packet */
		    case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
		      {
			GmMagElementStatusPacket *statusPacket = (GmMagElementStatusPacket*)&reply;
			HandleStatusPacket (statusPacket, counter, options, outputFile);
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


int RunUdpClient (MagElementTestOptions &options, FILE *outputFile)
{
  ip::udp::endpoint remote_endpoint;

  try {
    /* Basic asio setup */
    boost::asio::io_context io_context;
    ip::udp::endpoint ep(ip::udp::v4(),atoi(options.mRemotePort.data()));
    ip::udp::socket sock(io_context,ep);
    
    uint32_t counter = 0;
    
    while (true)
      {
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return(0);
	    }
	while (true)
	  {
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return(0);
	    }
	    int character = cin.peek();
	    if (character == 'q')
	      {
		break;
	      }
	    /* Buffer into which data will be read.  It is as long or longer than
	       the longest expected packet; this means that if the instrument and the
	       network are functioning correctly, it should always include the header
	       for at least one valid record, and once the program has locked on
	       to the packet sequence this buffer can hold an entire packet of any
	       valid type. */
	    uint8_t reply[max_length];
            
	    bool found = false;
	    found = false;

	    /* Look for heartbeat records; we should always get one within about 1 second,
	       if the MagElement is functioning correctly */
	    int leftover = -1;

	    /* Read a heartbeat's worth of data into the the buffer. Eventually one will be 
	       found. */
	    size_t replyLength = sock.receive_from(boost::asio::buffer(reply, max_length),
						   remote_endpoint);
	    cout << replyLength << std::endl;
	    //size_t replyLength = boost::asio::read(s, boost::asio::buffer(reply, HEARTBEAT_LENGTH));

	    /* Does the buffer include a heartbeat header? */
	    int test = findStartOffset(reply, replyLength, (uint8_t *) heartbeatStart, HEARTBEAT_START_LENGTH,
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
	  if (sShutDown)
	    {
	      fclose (outputFile);
	      return(0);
	    }
	  counter++;
	  uint8_t reply[2000];

	  /* Read an 8-byte header. All records from MagElement include an 8-byte header.*/
	  typedef uint32_t IntAndSizeHeader[2];
	  IntAndSizeHeader* testHeader;
	  bool recognizedRecord = false;
	  size_t replyLength = sock.receive_from( boost::asio::buffer((uint8_t*)reply, max_length),remote_endpoint);

	  /* Found? */
	  if (replyLength >  8)
	    {
	      testHeader = (IntAndSizeHeader*)&reply;
	      switch ((*testHeader)[0])
		{
		  /* Is the type known? */
		case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
		case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
		case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
		  recognizedRecord = true;

		  /* Read the remaining part  of the packet */
		  switch ((*testHeader)[0])
		    {

		      /* 1000Hz block packet */
		    case GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS:
		      {
		      StreamerPacket *streamerPacket = (StreamerPacket *) &reply;
		      HandleRawDataBlock (streamerPacket, counter, options, outputFile);
		      }
		      break;

		      /* Decimated magnetometer packet */
		    case GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT:
		      {
		      IndexedMagElementDecimatedMagPacketWithHeader *decimatedPacket = (IndexedMagElementDecimatedMagPacketWithHeader *) &reply;
		      HandleDecimatedPacket (decimatedPacket, counter, options, outputFile);
		      }
		      break;

		      /* Heartbeat/status packet */
		    case GM_MAG_ELEMENT_HEARTBEAT_FORMAT:
		      {
		      GmMagElementStatusPacket *statusPacket = (GmMagElementStatusPacket*)&reply;
		      HandleStatusPacket (statusPacket, counter, options, outputFile);
		      }
		      break;
		    }
		  break;

		  
		default:
		  if (options.mVerboseMode)
		    {
		      cerr << "Unrecognized.\n";
		      break;
		    }
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



int main(int argc, char* argv[])
{
  std::cout << "MagElementTestLinux version " << EXAMPLE_VERSION << std::endl;

  MagElementTestOptions options {argc,argv};

  FILE *pFile = nullptr;
  if (options.mValid && options.mFileIsValid)
    {
      pFile = fopen (options.mFileNameToSave.data(),"wb");

      if (pFile == nullptr)
	{
	  std::cerr << "\n\nError: Output file "
		    << options.mFileNameToSave
		    << "can't be opened to save data\n\n";
	  options.mValid = false;
	} ;
    }

  if (!options.mValid)
    {
      std::cerr << sHelpText;
      return 0;
    }
  else
    {
      std::cerr << "Command line arguments are valid.\n";
      std::cout << "Press q,[ENTER] to exit.\n";
      std::cout << "Ctrl-C forces exit without saving output file.\n";
    }

  if (options.mLicenseRequest)
    {
      std::cout << licenseText;
      return 0;
    }
  else
    {
      std::thread commandWatcher(WaitForCommand);
      
      if (options.mAcceptUdp)
	{
	  return RunUdpClient (options, pFile);
	}
      else if (options.mAcceptTcp)
	{
	  return RunTcpClient (options, pFile);
	}
    }
  return 0;
}
