#ifndef __EOSBUILDER_DAQ__
#define __EOSBUILDER_DAQ__

/**
 * DAQ tools
 *
 * Interfacing for WbLSdaq clients.
 */

#include <evb/ds.h>
#include <stdint.h>

/**
 * @struct ChannelData
 * @
 */
typedef struct ChannelData {
  uint32_t chID;
  uint32_t offset;
  uint32_t threshold;
  float dynamic_range;
  uint16_t samples[20][200];
  uint16_t patterns[20];
} ChannelData;

/**
 * @struct DigitizerData
 * @
 */
typedef struct DigitizerData {
  uint16_t type;
  char name[50];
  uint16_t bits;
  uint16_t samples;
  uint16_t nEvents;
  float ns_sample;
  uint16_t channel_enabled_mask;
  uint32_t counters[20];
  uint32_t timetags[20];
  uint16_t exttimetags[20];
  ChannelData channels[16];
} DigitizerData;


// Make a CAENEvent by copying one event from a DigitizerData
CAENEvent* make_caenevent(int i, DigitizerData* caen, CAENEvent* e);


/**
 * Convert a WbLSdaq timestamp to a hash table key.
 *
 * @param timestamp The DAQ timestamp
 * @param ts If non-null, returns normalized high-precision timestamp
 * @return The normalized key
 */
uint64_t daq_key(uint64_t timestamp, uint64_t* ts);

/**
 * Convert string of digitizer serial number to its logical id
 *
 * @param name String of the digitizer serial number
 * @return The digitizer id
 */
int8_t digid_from_name(char* name);

/**
 * Parse a DAQ packet.
 * 
 * Fills DAQ data into the event hash table.
 *
 * @param data The data buffer
 */
void accept_daq(char* data);

#endif

