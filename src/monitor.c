#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <jemalloc/jemalloc.h>
#include <evb/monitor.h>
#include <evb/ds.h>
#include <evb/shipper.h>

extern Event* events;
extern Record* records;
extern Record* headers;
extern pthread_mutex_t record_lock;

extern uint32_t bytes_written;
extern uint32_t events_written;
extern char filename[100];

void handler(int signal);

float timediff(struct timespec t1, struct timespec t2) {
  return t2.tv_sec - t1.tv_sec + 1e-9 * (t2.tv_nsec - t1.tv_nsec);
}

void* monitor(void* ptr) {
  signal(SIGINT, &handler);

  struct timespec tw_n, tw_l;
  clock_gettime(CLOCK_MONOTONIC, &tw_l);
  clock_gettime(CLOCK_MONOTONIC, &tw_n);

  while(1) {
    clock_gettime(CLOCK_MONOTONIC, &tw_n);
    float dt = timediff(tw_l, tw_n);

    unsigned int queued = event_count();
    float wrate = dt != 0 ? bytes_written / dt : 0;
    float erate = dt != 0 ? events_written / dt : 0;
    int evt = events_written;
    bytes_written = 0;
    events_written = 0;
    clock_gettime(CLOCK_MONOTONIC, &tw_l);

    // Event check
    int nstale = 0;
    for (Event* e=events; e!=NULL; e=e->hh.next) {
      if (timediff(e->creation_time, tw_n) > 5 && !event_ready(e)) {
        nstale++;
        //printf("# stale key %li (ptb %i, caen %i)\n", e->id, e->ptb_status, e->caen_status);
        pthread_mutex_lock(&record_lock);
        record_push(&records, e->id, DETECTOR_EVENT, (void*)e);
        pthread_mutex_unlock(&record_lock);
      }
    }

    // Status message
    printf("%% q %i", queued);
    if (nstale > 0) {
      printf(" [%i!]", nstale);
    }
    printf(", w %i/%1.2fHz/%1.2fkBps => %s\n", evt, erate, wrate/1000, filename);

    sleep(5);
  }

  return NULL;
}

