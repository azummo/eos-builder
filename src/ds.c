#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <jemalloc/jemalloc.h>
#include <evb/config.h>
#include <evb/ds.h>

extern Config* config;
extern Event* events;
extern Record* records;
extern Record* headers;
extern pthread_mutex_t record_lock;

/** Events */

Event* event_create(uint64_t key) {
  Event* e = malloc(sizeof(Event));
  e->id = key;
  e->type = DETECTOR_EVENT;
  e->caen_status = 0;
  e->ptb_status = 0;
  if(!config->ptb_exists) {
    e->ptb.trigger_word = 1;
    e->ptb.timestamp = 0;
    e->ptb.word_type = 0;
  }
  pthread_mutex_init(&e->lock, NULL);
  clock_gettime(CLOCK_MONOTONIC, &e->creation_time);
  event_push(key, e);
  return e;
}

Event* event_at(uint64_t key) {
  Event* e;
  HASH_FIND(hh, events, &key, sizeof(uint64_t), e);
  return e;
}

void event_push(uint64_t key, Event* e) {
  HASH_ADD(hh, events, id, sizeof(uint64_t), e);
}

Event* event_pop(uint64_t key) {
  Event* e = event_at(key);
  if (e) HASH_DEL(events, e);
  return e;
}

void event_list() {
  for (Event* e=events; e!=NULL; e=e->hh.next) {
    printf("Event: %li\n", e->id);
  }
}

unsigned int event_count() {
  return HASH_COUNT(events);
}

uint8_t event_ready(Event* s) {
  if (!s) return false;
  return ((s->ptb_status || !config->ptb_exists) &&
          (s->caen_status & config->dig_mask) == config->dig_mask);
}

/** Runs. */

void accept_run_start(char* data) {
  RunStart* rs = (RunStart*) (data+4);
  rs->first_event_id /= config->evb_slice;
  record_push(&headers, rs->first_event_id, RUN_START, (void*)rs);
}

void accept_run_end(char* data) {
  RunEnd* re = (RunEnd*) (data+4);
  re->last_event_id /= config->evb_slice;
  record_push(&headers, re->last_event_id, RUN_END, (void*)re);
}

/** Output records. */

void record_push(Record** rec, uint64_t key, RecordType type, void* data) {
  Record* r = (Record*) malloc(sizeof(Record));
  r->id = key;
  r->type = type;
  r->data = data;
  HASH_ADD(hh, *rec, id, sizeof(uint64_t), r);
}

Record* record_at(Record** rec, uint64_t key) {
  Record* r;
  HASH_FIND(hh, *rec, &key, sizeof(uint64_t), r);
  return r;
}

Record* record_pop(Record** rec, uint64_t key) {
  Record* r = NULL;
  HASH_FIND(hh, *rec, &key, sizeof(uint64_t), r);
  if (r) HASH_DEL(*rec, r);
  return r;
}

unsigned int record_count(Record** rec) {
  return HASH_COUNT(*rec);
}

int64_t record_by_id(const Record* a, const Record* b) {
  return (a->id - b->id);
}

uint64_t record_next(Record** rec) {
  if (record_count(rec) == 0) return -1;
  pthread_mutex_lock(&record_lock);
  HASH_SORT(*rec, record_by_id);
  pthread_mutex_unlock(&record_lock);
  return (*rec)->id;
}

