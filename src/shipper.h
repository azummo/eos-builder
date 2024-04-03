#ifndef __EOSBUILDER_SHIPPER__
#define __EOSBUILDER_SHIPPER__

/**
 * The Shipper
 *
 * The shipper thread monitors the output queue and writes
 * out and/or dispatches completed events.
 */

#include <evb/ds.h>

#define MAX_RHDR_WAIT 100000*50

// Shipper
void* shipper(void* ptr);

// Send a whole packet, for a known size.
void send_all(int socket_handle, char* data, int size);

#endif

