#ifndef __EOSBUILDER_DS__
#define __EOSBUILDER_DS__

/**
 * Data structures
 *
 * Internal data structures for event building.
 * 
 * Events in memory are held in a uthash hash tables indexed by
 * a timestamp key derived from the system (PTB, V1730) counters.
 */

#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <evb/ptb.h>
#include <evb/uthash.h>

#define NDIGITIZERS 17

/** Output record types. */
typedef enum {
  EMPTY,
  DETECTOR_EVENT,
  RUN_START,
  RUN_END,
} RecordType;

/**
 * @struct CDABHeader
 *
 * A header that precedes each structure in a CDAB record.
 * Note: "CDAB" is just dumping structs to disk/network.
 */
typedef struct {
  uint32_t record_type;  //!< RecordType, per the enum
  uint32_t size;  //!< Record/packet size in bytes
} CDABHeader;


/** @struct CAENChannel */
typedef struct {
  uint32_t chID;
  uint32_t offset;
  uint32_t threshold;
  float dynamic_range;
  uint16_t samples[200];
  uint16_t pattern;
} CAENChannel;

/** @struct CAENEvent */
typedef struct {
  uint16_t type;
  char name[50];
  uint16_t bits;
  uint16_t samples;
  float ns_sample;
  uint16_t channel_enabled_mask;
  uint32_t counter;
  uint32_t timetag;
  uint16_t exttimetag;
  CAENChannel channels[16];
} CAENEvent;


/**
 * @struct Event
 * 
 * Contains all data for a single Eos detector event.
*/
typedef struct {
  uint64_t id;  //!< The key
  RecordType type;  //!< Type of record (event data)
  CAENEvent caen[NDIGITIZERS];  //!< Array of digitizer data
  ptb_trigger_t ptb;  //!< PTB record
  uint32_t caen_status;  //!< Bit mask indicating which V1730 data is filled
  uint8_t ptb_status;  //!< Boolean indicating whether PTB data is filled
  pthread_mutex_t lock;  //!< Lock for this record
  struct timespec creation_time;  //!< Wall time for event record creation
  UT_hash_handle hh;
} Event;

// Create a new event in the hash and return it
Event* event_create(uint64_t key);

// Get the event for a key
Event* event_at(uint64_t key);

// Push an Event into the queue at a given key
void event_push(uint64_t key, Event* data);

// Pop an Event, removing it from the queue and returning it
Event* event_pop(uint64_t key);

// Print a list of the queued events
void event_list();

// Return a count of the queue size
unsigned int event_count();

// Check whether all components of an event are populated
uint8_t event_ready(Event* s);

/**
 * @struct RHDR
 *
 * RHDR: SNO/SNO+ run header
*/
typedef struct {
  uint32_t type;
  uint32_t date;
  uint32_t time;
  uint32_t daq_ver;
  uint32_t runmask;
  uint64_t first_event_id;
  uint32_t run_id;
} RunStart;

typedef struct {
  uint32_t type;
  uint32_t date;
  uint32_t time;
  uint32_t daq_ver;
  uint32_t runmask;
  uint64_t last_event_id;
  uint32_t run_id;
} RunEnd;

// Handle run start header
void accept_run_start(char* data);

// Handle run end header
void accept_run_end(char* data);

/**
 * @struct Record
 *
 * Outgoing record queue. Events are placed in this queue
 * when they are completed, and will be popped from the queue
 * in key order.
 */
typedef struct {
  uint64_t id;  //!< The key
  RecordType type;  //!< Type of record (event, header)
  void* data;  //!< The record data
  UT_hash_handle hh;
} Record;

// Push a Record onto the queue
void record_push(Record** rec, uint64_t key, RecordType type, void* data);

// Access a Record
Record* record_at(Record** rec, uint64_t key);

// Pop a record from the queue
Record* record_pop(Record** rec, uint64_t key);

// Return a count of the queue size
unsigned int record_count(Record** rec);

// Get the key for the next record to process (key sorted)
uint64_t record_next(Record** rec);

#endif

