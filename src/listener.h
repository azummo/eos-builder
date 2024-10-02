#ifndef __EOSBUILDER_LISTENER__
#define __EOSBUILDER_LISTENER__

/**
 * The Listener
 *
 * The listener thread accepts incoming connections from DAQ
 * components and parses their messages to fill the event
 * queue. Each client is managed in a separate thread.
 */

#include <evb/ds.h>

#define MAX_BUFFER_LEN 350000

/**
 * @struct tcp_header
 *
 * Generic 4 byte packet header.
 */
typedef struct {
  uint16_t type;
  uint16_t size;
} tcp_header;


/**
 * @struct time_offsets
 *
 * Time offsets for all DAQ system components. We presume that all
 * system components have a random offset in their timestamps, and
 * discover it using the timestamps for the first event received
 * in a run.
 *
 * Sometimes we also see flipped bits in the (VST) PTB timestamps,
 * so we enforce monotonicity and keep track of an incrementing
 * offset to account for this.
 */
typedef struct {
  uint64_t ptb_last;
  uint64_t ptb_dt;
  uint64_t ptb;
  uint64_t caen[NDIGITIZERS];
} time_offsets;


void close_sockets();
void handler(int signal);
void die(const char *msg);
void* listener_child(void* psock);
void* listener(void* ptr);

/** Labels for received packet types. */
typedef enum {
  CTL_PACKET,
  CMD_PACKET,
  PTB_PACKET,
  DAQ_PACKET,
  RUN_START_PACKET,
  RUN_END_PACKET,
  UNK_PACKET,
} PacketType;

/** Helpers */

// Get a whole packet, for a known size.
void recv_all(int socket_handle, char* dest, int size);

// Try to ID packet type based on the beginning.
PacketType packet_id(char* h);

#endif

