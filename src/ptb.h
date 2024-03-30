#ifndef __EOSBUILDER_PTB__
#define __EOSBUILDER_PTB__

/**
 * PTB tools
 *
 * Interfacing for Penn Trigger Board clients.
 */

#include <stdint.h>

// Minimal SBND-style ptb_content structs

enum ptb_word_type {
  ptb_t_fback=0x0, ptb_t_lt=0x1, ptb_t_gt=0x2, ptb_t_ch=0x3, ptb_t_chksum=0x4, ptb_t_ts=0x7
};

typedef struct {
  uint16_t packet_size : 16;
  uint8_t sequence_id : 8;
  uint8_t format_version : 8;
} ptb_tcp_header_t;

typedef struct {
  uint64_t timestamp;
  uint64_t payload : 61;
  uint64_t word_type : 3;
} ptb_word_t;

typedef struct {
  uint64_t timestamp;
  uint64_t code : 8;
  uint64_t source : 8;
  uint64_t payload1 : 16;
  uint64_t payload2 : 29;
  uint64_t word_type : 3;
} ptb_feedback_t;

typedef struct {
  uint64_t timestamp : 61;
  uint64_t beam : 3;
  uint64_t crt : 14;
  uint64_t pds : 10;
  uint64_t mtca : 6;
  uint64_t nim : 6;
  uint64_t auxpds : 25;
  uint64_t word_type : 3;
} ptb_ch_status_t;

typedef struct {
  uint64_t timestamp;
  uint64_t padding : 61;
  uint64_t word_type : 3;
} ptb_timestamp_t;

typedef struct {
  uint64_t timestamp;
  uint64_t trigger_word : 61;
  uint64_t word_type : 3;
} ptb_trigger_t;

/**
 * Convert a PTB timestamp to a hash table key.
 *
 * @param timestamp The PTB timestamp
 * @param ts If non-null, returns normalized high-precision timestamp
 * @return The normalized key
 */
uint64_t ptb_key(uint64_t timestamp, uint64_t* ts);

/**
 * Parse a PTB packet.
 * 
 * Fills PTB data into the event hash table.
 *
 * @param data The data buffer
 */
void accept_ptb(char* data);

#endif

