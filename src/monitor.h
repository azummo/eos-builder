#ifndef __EOSBUILDER_MONITOR__
#define __EOSBUILDER_MONITOR__

/**
 * The Monitor
 *
 * The monitor thread watches the event queue to report
 * status information and check for various pathologies.
 */

// Monitor
void* monitor(void* ptr);

#endif

