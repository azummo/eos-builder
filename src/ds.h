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
#include <evb/daq.h>
#include <evb/ptb.h>
#include <evb/uthash.h>

#define DIGITIZERS 0x1
#define NDIGITIZERS 1

typedef enum {
  EMPTY,
  DETECTOR_EVENT,
  RUN_HEADER,
} RecordType;

/**
 * @struct Event
 * 
 * Contains all data for a single Eos detector event.
*/
typedef struct {
  uint64_t id;  //!< The key
  RecordType type;  //!< Type of record (event data)
  DigitizerData caen[NDIGITIZERS];  //!< Array of digitizer data
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
void record_push(uint64_t key, RecordType type, void* data);

// Pop a record from the queue
Record* record_pop(uint64_t key);

// Return a count of the queue size
unsigned int record_count();

// Get the key for the next record to process (key sorted)
uint64_t record_next();


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
  uint32_t calib_trial_id;
  uint32_t srcmask;
  uint32_t runmask;
  uint32_t cratemask;
  uint32_t first_event_id;
  uint32_t valid_event_id;
  uint32_t run_id;
} RHDR;

#endif

