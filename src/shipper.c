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
#include <evb/shipper.h>
#include <evb/ds.h>

extern Event* events;

extern Record* records;
extern pthread_mutex_t record_lock;

uint32_t bytes_written;
uint32_t events_written;

void handler(int signal);

char filename[100] = "";
FILE* outfile;


void* shipper(void* ptr) {
  outfile = NULL;
  int run_id = 0;
  int start_key = 0;

  sprintf(filename, "run_%i.cdab", 0);
  printf("> Run %i, key %i => %s\n", run_id, start_key, filename);
  outfile = fopen(filename, "wb+");

  signal(SIGINT, &handler);
  while(1) {
    uint64_t key = record_next();

    if (key == -1) {
      sleep(1);
      continue;
    }

    Record* r = record_pop(key);
    if (!r) {
      printf("popped null Record pointer!? key=%li\n", key);
      continue;
    }

    if (r->type == DETECTOR_EVENT) {
      Event* e = event_pop(key);

      if (!e) {
        printf("popped null Event pointer!? key=%li\n", key);
        continue;
      }

      if (!event_ready(e)) {
        printf("# write partial key %li (ptb %i, caen %i)\n",
               e->id, e->ptb_status, e->caen_status);
      }

      CDABHeader cdh;
      cdh.record_type = DETECTOR_EVENT;
      cdh.size = sizeof(Event);
      fwrite(&cdh, sizeof(CDABHeader), 1, outfile);
      fwrite(e, sizeof(Event), 1, outfile);
      free(e);
      bytes_written += sizeof(Event) + sizeof(CDABHeader);
      events_written++;
    }

    else if (r->type == RUN_HEADER) {
      Record* h = record_pop(key);

      if (!h) {
        printf("popped null Header pointer!? key=%li\n", key);
        continue;
      }

      printf("write a header\n");
    }

    else {
      printf("# popped unknown record type %i\n", r->type);
    }
  }
}

