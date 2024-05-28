#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <jemalloc/jemalloc.h>
#include <evb/config.h>
#include <evb/listener.h>
#include <evb/shipper.h>
#include <evb/monitor.h>
#include <evb/ds.h>

/**
 * Event Builder for Eos, C edition
 *
 * Enqueues incoming raw data in hash table buffers, and
 * writes out to disk and/or a socket as events are finished.
 *
 * Andy Mastbaum (mastbaum@physics.rutgers.edu), March 2024
 * Andy Mastbaum (mastbaum@hep.upenn.edu), June 2011
 */ 

Config* config = NULL;
Event* events = NULL;
Record* records = NULL;
Record* headers = NULL;
pthread_mutex_t record_lock;

int main(int argc, char* argv[]) {
    printf("* eos-builder\n");

    if(argc != 2) {
        printf("Usage: %s <config.json>\n", argv[0]);
        exit(1);
    }
    else {
      config = config_parse(argv[1]);
      config_print(config);
    }

    pthread_mutex_init(&record_lock, NULL);

    // fake RunStart for testing
    RunStart* rhdr = (RunStart*) malloc(sizeof(RunStart));
    rhdr->type = RUN_START;
    rhdr->run_id = 0;
    record_push(&headers, 0, RUN_START, rhdr);

    // launch listener (input), shipper (output), monitor threads
    pthread_t tlistener;
    pthread_create(&tlistener, NULL, listener, (void*)&config->evb_port);
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

