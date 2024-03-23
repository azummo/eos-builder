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
    //AV_STATUS_HEADER,
    //MANIPULATOR_STATUS_HEADER,
    //TRIG_BANK_HEADER,
    //EPED_BANK_HEADER
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
  uint64_t id;  // Key = timetag / 10?
  uint64_t timetag;
  uint32_t gtid;

  uint32_t caen_status;
  bool ptb_status;

  DigitizerData caen[NDIGITIZERS];
  ptb_trigger_t ptb;

  clock_t builder_arrival_time;
  uint32_t run_id;
  uint32_t subrun_id;
  uint32_t nhits;
  uint8_t mcflag;
  uint8_t datatype;

  pthread_mutex_t lock;

  UT_hash_handle hh;
} Event;

Event* event_at(uint64_t key);
Event* event_push(uint64_t key);
Event* event_pop(uint64_t key);

bool event_ready(Event* s);
//bool event_ready(int key);
int event_write(uint64_t key, char* dest);

void event_list();
unsigned int event_count();


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


/*
typedef struct
{
    uint64_t write;
    uint64_t read;
    uint64_t offset; // index-gtid offset (first gtid)
    uint64_t size;
    void** keys;
    RecordType* type;

    pthread_mutex_t* mutex_buffer; // lock elements individually
    pthread_mutex_t mutex_write;
    pthread_mutex_t mutex_read;
    pthread_mutex_t mutex_offset;
    pthread_mutex_t mutex_size;
} Buffer;

// allocate memory for and initialize a ring buffer
Buffer* buffer_alloc(Buffer** pb, int size);

// print buffer status information for debugging
void buffer_status(Buffer* b);

// re-initialize a buffer; frees memory held by (pointer) elements
void buffer_clear(Buffer* b);

// returns the array index corresponding to gtid id
uint64_t buffer_keyid(Buffer* b, unsigned int id);

// get an element out of the buffer at gtid id
int buffer_at(Buffer* b, unsigned int id, RecordType* type, void** pk);

// insert an element into the buffer at gtid id. mutex locking done by user.
int buffer_insert(Buffer* b, unsigned int id, RecordType type, void* pk);

int buffer_isfull(Buffer* b);
int buffer_isempty(Buffer* b);

int buffer_push(Buffer* b, RecordType type, void* key);
int buffer_pop(Buffer* b, RecordType* type, void** pk);
*/

