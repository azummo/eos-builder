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

// CDABHeader: a header that precedes each structure in a CDAB file
typedef struct {
  uint32_t record_type;
  uint32_t size;
} CDABHeader;

// Shipper
void* shipper(void* ptr);

#endif

