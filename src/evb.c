#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <jemalloc/jemalloc.h>
#include <evb/listener.h>
#include <evb/shipper.h>
#include <evb/monitor.h>
#include <evb/ds.h>

/** Event Builder for Eos, C edition
 *
 *  Enqueues incoming raw data in ring buffers, and writes out to disk and/or
 *  a socket as events are finished.
 *
 *  Andy Mastbaum (mastbaum@physics.rutgers.edu), March 2024
 *  Andy Mastbaum (mastbaum@hep.upenn.edu), June 2011
 */ 

Event* events = NULL;

int main(int argc, char* argv[]) {
    int port;
    if(argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    else
        port = atoi(argv[1]);

    // fake RHDR for testing
    RHDR* rh = (RHDR*) malloc(sizeof(RHDR));
    rh->run_id = 123456;
    rh->first_event_id = 0;
    //buffer_push(run_header_buffer, RUN_HEADER, rh);

    // launch listener (input), shipper (output), monitor threads
    pthread_t tlistener;
    pthread_create(&tlistener, NULL, listener, (void*)&port);
    pthread_t tshipper;
    pthread_create(&tshipper, NULL, shipper, NULL);
    pthread_t tmonitor;
    pthread_create(&tmonitor, NULL, monitor, NULL);

    // wait for threads to join before exiting
    pthread_join(tlistener, NULL);
    pthread_join(tshipper, NULL);
    pthread_join(tmonitor, NULL);

    return 0;
}

