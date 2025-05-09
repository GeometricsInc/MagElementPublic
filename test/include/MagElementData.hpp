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
#ifndef MAGELEMENT_DATA_HPP_
#define MAGELEMENT_DATA_HPP_

#include <utility>
#include <atomic>
#include <stdint.h>
#include "gmplatform.h"

#ifndef DWORD
typedef unsigned long DWORD;
#endif

// Conversion from integer mag data to float.
#define MFAM_NANOTESLAS_PER_LSB 5.0E-5
#define MAG_DATA_AS_FLOAT(x)  (1.0*(x)*MFAM_NANOTESLAS_PER_LSB)
#define GM_DATA_DOMAIN_MAGNETOMETER (0x01 << 16)

/* Type identifier for the MagElement 1000Hz, 40-record raw MFAM data, with header info including type and size. */
#define GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS           (GM_DATA_DOMAIN_MAGNETOMETER | 0x1F)

/* Indentifer for the MagElement filtered data record */
#define GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT    (GM_DATA_DOMAIN_MAGNETOMETER | 0x22)

/* Indentifier for the MagElement heartbeat/status record */
#define GM_MAG_ELEMENT_HEARTBEAT_FORMAT           (GM_DATA_DOMAIN_MAGNETOMETER | 0x23)

/*  MFAM data structure. */
PACKED_PRAGMA
typedef struct PACKED_SPEC s_MfamSpiPacket {
  /** \brief Union of a few different data types, including the
   * fiducial number.  Retrieve the fiducial with the macro GET_FID_COUNT */
  uint16_t frameid;
    
  uint16_t sysstat;
    
  uint32_t mag1data;
  uint16_t mag1stat;
  uint16_t mag2stat;
  uint32_t mag2data;
    
  // Auxiliary fields.
  uint16_t auxsenx;
  uint16_t auxseny;
  uint16_t auxsenz;
  uint16_t auxsent;
   
} MfamSpiPacket ALIGN_1_SPEC;


/* Basic MFAM packet, but in floating point form, for decimated
   data formats */
PACKED_PRAGMA
typedef struct PACKED_SPEC s_MfamFloatPacket {
  /** \brief Union of a few different data types, including the fiducial number.
   * which can be retrieved with macro GET_FID_COUNT.
   * Offset zero, length 2 */
  uint16_t frameid;
  /** \brief sysstat - Information about system status. Offset 2, length 2 */
  uint16_t sysstat;
  double mag1data;     // Offset 4, length 8
  uint16_t mag1stat;   // Offset 12, length 2
  uint16_t mag2stat;   // Offset 14, length 2
  double mag2data;     // Offset 16, length 8
    
  /* Auxiliary fields. */
  float  mTemperature,     // Offset 24, length 4
    mCompassX,        // Offset 28, length 4
    mCompassY,        // Offset 32, length 4
    mCompassZ,        // Offset 36, length 4
    mGyroX,           // Offset 40, length 4
    mGyroY,           // Offset 44, length 4
    mGyroZ,           // Offset 48, length 4
    mAccelerometerX,  // Offset 52, length 4
    mAccelerometerY,  // Offset 56, length 4
    mAccelerometerZ,  // Offset 60, length 4
    mAgcGain1,        // Offset 64, length 4
    mAgcGain2;        // Offset 68, length 4
} MfamFloatPacket ALIGN_1_SPEC; // Structure size is 72

/*  Structure to index a packet */
PACKED_PRAGMA
typedef struct PACKED_SPEC s_PacketIndex
{
  uint64_t   uiPacketIndex;  // Offset 0, length 8
} PacketIndex;  // Packet size is 8

// -------- MFAM Filtered packet with indexing added.  -----------------
struct PACKED_SPEC IndexedMfamFilteredPacket {
  PacketIndex      piIndex;                // Offset 0, length 8
  MfamFloatPacket  mMfamFilteredPacket;    // Offset 8, length 72
} ALIGN_1_SPEC;  //  structure size is 80

PACKED_PRAGMA
typedef struct PACKED_SPEC s_IndexedMfamFilteredPacketWithHeader
{
  /* dwRecordType should be GM_MFAM_FILTERED_INDEXED_W_HEADER */
  DWORD                      dwRecordType;  // Offset 0, length 4
  uint32_t                   uRecordSize; // Offset 4, length 4
  IndexedMfamFilteredPacket  imspData;      // Offset 8, length 80
} IndexedMfamFilteredPacketWithHeader ALIGN_1_SPEC;  // structure size is 88


//#warning These should be changed to hard-coded numbers, with asserts to verify that they match sizeof, \
  so that compiler packing is getting checked at compile time.
//#define GM_SIZEOF_INDEXED_SPI_W_HEADER (sizeof(IndexedMfamSpiPacketWithHeader))
#define GM_SIZEOF_INDEXED_MFAM_FILTERED_W_HEADER (sizeof(IndexedMfamFilteredPacketWithHeader))

/* Number of MFAM records in a block packet */
#define MFAM_STREAMER_CACHE_SIZE 40
  
// Four 16-bit A2D measurements, packaged together.
PACKED_PRAGMA
typedef struct PACKED_SPEC s_A2d16Quadruple
{
  uint16_t adc0;
  uint16_t adc1;
  uint16_t adc2;
  uint16_t adc3;
} A2d16Quadruple ALIGN_1_SPEC;
  
/* Raw MFAM data with ADC data attached. */  
PACKED_PRAGMA
typedef struct PACKED_SPEC _MfamPlusAnalogQuad
{
  MfamSpiPacket          mMagData;  // Size 24, offset 0
  A2d16Quadruple       mAnalogs;  /* Size 8, offset 24 */
} MfamPlusAnalogQuad ALIGN_1_SPEC;  /* structure size 32 */

/* Decimated MFAM record format */
PACKED_PRAGMA
typedef struct PACKED_SPEC s_MagElementDecimatedMagPacket {
  double mMagData;
  float  mFieldStrength;
  uint32_t mDataValid;
  float    mCompassX,
    mCompassY,
    mCompassZ,
    mAccelerometerX,
    mAccelerometerY,
    mAccelerometerZ,
    mGyroX,
    mGyroY,
    mGyroZ,
    mImuTemp;
  uint8_t  mReserved[16];
} MagElementDecimatedMagPacket ALIGN_1_SPEC;


/* Decimated MFAM record with an index. */
PACKED_PRAGMA
typedef struct  PACKED_SPEC s_IndexedMagElementDecimatedMagPacket
{
  uint64_t                     mIndex;
  MagElementDecimatedMagPacket mPacket;
} IndexedMagElementDecimatedMagPacket ALIGN_1_SPEC; /* Size 80 */


/* Decimated MFAM record, with header and index. This is the 
   MagElement decimated record. */
PACKED_PRAGMA
typedef struct  PACKED_SPEC s_IndexedMagElementDecimatedMagPacketWithHeader
{
    
  uint32_t         mRecordType, /* GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT */
                   mRecordSize; /* Size 88 */
  IndexedMagElementDecimatedMagPacket mIndexedPacket;
} IndexedMagElementDecimatedMagPacketWithHeader ALIGN_1_SPEC; /* Size 88 */

#define SIZE_OF_GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT (88)

/* Verify that the compiler is handling packing correctly.  Use the hard-coded number! */
static_assert ((sizeof(IndexedMagElementDecimatedMagPacketWithHeader) == SIZE_OF_GM_MAG_ELEMENT_DECIMATED_OUTPUT_FORMAT),
               "Not expected size");
  

/* The MagElement 1000Hz data block */
PACKED_PRAGMA
class PACKED_SPEC  StreamerPacket
{
public:
  union {
    char  mHeader[16];
    struct {
      uint32_t mRecordType; /* GM_MFAM_DEVKIT_BLOCK_WITH_EMPTY_ADCS_NO_GPS */
      uint32_t mRecordSize; /* sizeof (StreamerPacket)        */
      uint64_t mFirstPacketIndex;
    } mStructuredHeader;
  };
  MfamPlusAnalogQuad  mDataBlock[MFAM_STREAMER_CACHE_SIZE];
};
  

/****************************************************/
/* Some useful MFAM data values, masks, and types.              */
/****************************************************/
#define FID_COUNT_MASK 0x7FF

#define MAG_1_VALID    0x4000
#define MAG_2_VALID    0x8000

/* MFAM records cycle through several types
 *  that are specified in frameid,
 *  in the bits masked in AUX_DATA_MASK  */
#define AUX_DATA_MASK  0x3800

/* Record types specified
 *  in (frameid & AUX_DATA_MASK) 
 */
#define COMPASS_MASK   0x800
#define SERIAL_MASK    0x3800
#define GYRO_MASK      0x1000
#define ACCEL_MASK     0x2000
#define SERIAL_V_MASK  0x3800

#define VALID_MFAM_FID_TEMPLATE(INDEX) MAG_2_VALID | MAG_1_VALID | INDEX

#define IS_MAG1_VALID(x)  ((x) & MAG_1_VALID)
#define IS_MAG2_VALID(x)  ((x) & MAG_2_VALID)


/** Get the fiducial number of an MFAM record.
 * Example:
 *     MfamSpiPacket *packet  = GetAPacketSomeHow ();
 *     uint16_t counter       = GET_FID_COUNT (packet->frameid);
*/
#define GET_FID_COUNT(x)  ((x) & FID_COUNT_MASK)

/** Macros to detect the record type of an MFAM record.
 * Example:
 *     MfamSpiPacket *packet  = GetAPacketSomeHow ();
 *     bool   isCompassRecord = IS_COMPASS (packet->frameid);
*/
#define IS_COMPASS(x)     ((((x) & AUX_DATA_MASK)==COMPASS_MASK)
#define IS_GYRO(x)        ((((x) & AUX_DATA_MASK)==GYRO_MASK)
#define IS_ACCEL(x)       (((x)  & AUX_DATA_MASK)==ACCEL_MASK)
#define IS_SERIAL(x)      (((x)  & AUX_DATA_MASK)==SERIAL_V_MASK)

/** Bitmasks in MfamSpiPacket.sysstat */
#define LOCK_MASK    0x4000
#define PPS_MASK     0x8000
#define FAILURE_MASK 0x1

/** Macros to get status of an MFAM record.
 * Example:
 *     MfamSpiPacket *packet  = GetAPacketSomeHow ();
 *     bool   isPpsReceived   = IS_PPS_RECEIVED (packet->sysstat);
*/
#define IS_PPS_RECEIVED(x)   (((x) & PPS_MASK)
#define IS_PPS_LOCKED(x)     (((x) & LOCK_MASK)
#define IS_MAG_FAILED(x)     (((x) & FAILURE_MASK)

/* Check individual mag status, for example:
 *  MfamSpiPacket *packet  = GetAPacketSomehow ();
 *  bool sensorOneIsDeadZone = IS_DEAD_ZONE (packet->mag1stat);
*/
#define DEAD_ZONE_MASK       0x0001
#define IS_DEAD_ZONE(x)     (((x) & DEAD_ZONE_MASK) == DEAD_ZONE_MASK)

#define GET_MAG(x)           (double(x) * MAG_TO_NT)

/* ================================================
 * System Status Masks for the sysstat field,
 *   in MfamSpiPacket.
*/
#define MAIN_STATE_MASK          0x3C00
#define SUB_STATE_MASK           0x0380
#define COMPASS_FAILURE_MASK     0x0380
#define STARTUP_FAIL_MASK        0x0001
#define MAIN_MODE_MASK           0x0007
#define MFAM_IN_STARTUP          0x0002
#define MFAM_IN_MAG_MODE         0x0004

// ============== System states.  These are application
//    values, not the values straight off of the bus
#define MAG_IS_STARTING(x)     ((x)&MFAM_IN_STARTUP)
#define MAG_IS_RUNNING(x)      ((x)&MFAM_IN_MAG_MODE)


PACKED_PRAGMA
class PACKED_SPEC GmMagElementStatusPacket
{
public:
  uint32_t mRecordType = GM_MAG_ELEMENT_HEARTBEAT_FORMAT; /* Offset 0 */
  uint32_t mRecordSize = sizeof(GmMagElementStatusPacket);/* Offset 4 */
  uint64_t mIndex;/* Offset 8 */
  uint32_t mIpAddress;/* Offset 16 */
  uint8_t  mSerialNumber[8];/* Offset 20 */
  uint8_t  mMfamSerialNumber[8];/* Offset 28 */
  uint32_t mSamplePeriod; /* SamplePeriod in mS *//* Offset 36 */
  uint32_t mMfamRunningMode;/* Offset 40 */
  uint32_t mPpsStatus;/* Offset 44 */
  uint64_t mCounterAtLastPps;/* Offset 48 */
  uint64_t mCounterAtFirstPps;/* Offset 56 */
  uint16_t mSupplyVoltage; /* Raw ADC value from last ADC2 *//* Offset 64 */
  uint16_t mLeakDetector;  /* Raw ADC value from last ADC3 *//* Offset 66 */
  uint16_t mAuxPortStatus;/* Offset 68 */
  uint16_t mTotalRunTime;  /* Total run time from MfamAux fields *//* Offset 70 */
  uint16_t mFpgaTemperature;/* Offset 72 */
  uint16_t mBoardTemperature;/* Offset 74 */
  uint32_t mSystemFaults; /* TBD: SD card status, etc.? *//* Offset 76 */
  uint32_t mReserved2;   /* Fault bits, System FaultID, Mag1FaultID, Mag2FaultID *//* Offset 80 */
  uint16_t mMfamStatus[4];   /* FrameId, SysStat, Mag1Stat and Mag2Stat from MFAM *//* Offset 84 */
  uint8_t  mReserved1[16];/* Offset 92 */
} ALIGN_1_SPEC ; /* Size 108 */

/* Check on compiler packing */
static_assert ((sizeof(GmMagElementStatusPacket) == 108),"Not expected size");
  
#endif /* GMMFAMDATA_HPP_ */

