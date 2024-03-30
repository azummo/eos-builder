#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <jemalloc/jemalloc.h>
#include <evb/ds.h>

extern Event* events;
extern Record* records;
extern pthread_mutex_t record_lock;

/** Events */

Event* event_create(uint64_t key) {
  Event* e = malloc(sizeof(Event));
  e->id = key;
  e->type = DETECTOR_EVENT;
  e->caen_status = 0;
  e->ptb_status = 0;
  pthread_mutex_init(&e->lock, NULL);
  clock_gettime(CLOCK_MONOTONIC, &e->creation_time);
  event_push(key, e);
  return e;
}

Event* event_at(uint64_t key) {
  Event* e;
  HASH_FIND(hh, events, &key, sizeof(uint64_t), e);
  //printf("event_at %i, s=%llx, size=%u\n", (int) key, s, HASH_COUNT(events));
  return e;
}

void event_push(uint64_t key, Event* e) {
  HASH_ADD(hh, events, id, sizeof(uint64_t), e);
  //printf("event_push %i, size=%u\n", (int) key, HASH_COUNT(events));
  //return e;
}

Event* event_pop(uint64_t key) {
  Event* e = event_at(key);
  if (e) HASH_DEL(events, e);
  //printf("event_pop %i, size=%u\n", (int) key, HASH_COUNT(events));
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
  return (s->ptb_status && (s->caen_status & DIGITIZERS) == DIGITIZERS);
}

/** Output records. */

void record_push(uint64_t key, RecordType type, void* data) {
  Record* r = (Record*) malloc(sizeof(Record));
  r->id = key;
  r->type = type;
  r->data = data;
  HASH_ADD(hh, records, id, sizeof(uint64_t), r);
}

Record* record_pop(uint64_t key) {
  Record* r = NULL;
  HASH_FIND(hh, records, &key, sizeof(uint64_t), r);
  if (r) HASH_DEL(records, r);
  return r;
}

unsigned int record_count() {
  return HASH_COUNT(records);
}

int64_t record_by_id(const Record* a, const Record* b) {
  return (a->id - b->id);
}

uint64_t record_next() {
  if (record_count() == 0) return -1;
  pthread_mutex_lock(&record_lock);
  HASH_SORT(records, record_by_id);
  pthread_mutex_unlock(&record_lock);
  return records->id;
}

