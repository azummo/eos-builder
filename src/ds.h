#ifndef __EOSBUILDER_DS__
#define __EOSBUILDER_DS__

#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <evb/ptb.h>
#include <evb/uthash.h>

/** Data structure structs */

#define DIGITIZERS 0x1
#define NDIGITIZERS 1

uint32_t get_bits(uint32_t x, uint32_t position, uint32_t count);

typedef enum {
  EMPTY,
  DETECTOR_EVENT,
  RUN_HEADER,
} RecordType;

/**
 * @struct ChannelData
 * @
 */
typedef struct ChannelData {
  uint32_t chID;
  uint32_t offset;
  uint32_t threshold;
  float dynamic_range;
  uint16_t samples[20][500];
  uint16_t patterns[20];
} ChannelData;

/**
 * @struct DigitizerData
 * @
 */
typedef struct DigitizerData {
  uint16_t type;
  uint16_t bits;
  uint16_t samples;
  uint16_t nEvents;
  float ns_sample;
  uint32_t counters[20];
  uint32_t timetags[20];
  uint16_t exttimetags[20];

  ChannelData channels[16];
} DigitizerData;

/**
 * @struct Event
 * 
 * Contains all data for a single Eos detector event.
*/
typedef struct {
  uint64_t id;
  RecordType type; 
  DigitizerData caen[NDIGITIZERS];
  ptb_trigger_t ptb;
  uint32_t caen_status;
  bool ptb_status;
  pthread_mutex_t lock;
  struct timespec creation_time;
  UT_hash_handle hh;
} Event;

Event* event_at(uint64_t key);
void event_push(uint64_t key, Event* data);
Event* event_pop(uint64_t key);
int event_write(uint64_t key, char* dest);
void event_list();
unsigned int event_count();
bool event_ready(Event* s);


/**
 * @struct Header
 * 
 * Header information for run/event metadata.
 */
typedef struct Header {
  uint64_t id;
  RecordType type;
  void* data;
  pthread_mutex_t lock;
  struct timespec creation_time;
  UT_hash_handle hh;
} Header;

Header* header_at(uint64_t key);
void header_push(uint64_t key, Header* data);
Header* header_pop(uint64_t key);
int header_write(uint64_t key, char* dest);
void header_list();
unsigned int header_count();

/// RHDR: run header
typedef struct
{
    uint32_t type;
    uint32_t date;
    uint32_t time;
    uint32_t daq_ver;
    uint32_t calib_trial_id;
    uint32_t srcmask;
    uint32_t runmask;
    uint32_t cratemask;
    uint32_t first_event_id;
    uint32_t valid_event_id;
    uint32_t run_id;
} RHDR;

#endif

