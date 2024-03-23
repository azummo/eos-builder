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

extern Event* events;

extern uint32_t bytes_written;
extern uint32_t events_written;
extern char filename[100];

void handler(int signal);

FILE* outfile;

void* monitor(void* ptr) {
  signal(SIGINT, &handler);

  struct timespec tw_n, tw_l;
  clock_gettime(CLOCK_MONOTONIC, &tw_l);
  clock_gettime(CLOCK_MONOTONIC, &tw_n);

  while(1) {
    clock_gettime(CLOCK_MONOTONIC, &tw_n);

    float dt = tw_n.tv_sec - tw_l.tv_sec + 1e-9 * (tw_n.tv_nsec - tw_l.tv_nsec);

printf("dt=%f\n", dt);
    unsigned int queued = event_count();
    float wrate = dt != 0 ? bytes_written / dt : 0;
    float erate = dt != 0 ? events_written / dt : 0;
    int evt = events_written;
    bytes_written = 0;
    events_written = 0;

    clock_gettime(CLOCK_MONOTONIC, &tw_l);

    int nstale = 0;
    for (Event* s=events; s!=NULL; s=s->hh.next) {
      if ((s->builder_arrival_time - 0) / CLOCKS_PER_SEC > 5) {
        nstale++;
        printf("# stale key %li (ptb %i, caen %i)\n", s->id, s->ptb_status, s->caen_status);
      }
    }

    printf("%% q %i", queued);
    if (nstale > 0) {
      printf(" [%i!]", nstale);
    }
    printf(", w %i %1.2fHz %1.2fkBps => %s\n", evt, erate, wrate/1000, filename);

    sleep(5);
  }

  return NULL;
}

