#include <stdio.h>
#include <evb/ds.h>
#include <evb/daq.h>
#include <evb/listener.h>

extern Event* events;
extern Record* records;

extern pthread_mutex_t record_lock;
extern time_offsets offsets;

CAENEvent* make_caenevent(int i, DigitizerData* caen, CAENEvent* e) {
  if (e == NULL) {
    e = (CAENEvent*) malloc(sizeof(CAENEvent));
  }

  e->type = caen->type;
  snprintf(e->name, 50, "%s", caen->name);
  e->bits = caen->bits;
  e->samples = caen->samples;
  e->ns_sample = caen->ns_sample;
  e->counter = caen->counters[i];
  e->timetag = caen->timetags[i];
  e->exttimetag = caen->exttimetags[i];

  for (int j=0; j<16; j++) {
    e->channels[j].chID = caen->channels[j].chID;
    e->channels[j].offset = caen->channels[j].offset;
    e->channels[j].threshold = caen->channels[j].threshold;
    e->channels[j].dynamic_range = caen->channels[j].dynamic_range;
    e->channels[j].pattern = caen->channels[j].patterns[i];
    memcpy(e->channels[j].samples, caen->channels[j].samples[i], 500*sizeof(uint16_t));
  }

  return e;
}

uint64_t daq_key(uint64_t timestamp, uint64_t* ts) {
  uint64_t t = timestamp;
  if (ts) *ts = t;
  return t / 4;
}

void accept_daq(char* data) {
  DigitizerData* p = (DigitizerData*) (data+4);
  uint8_t digid = 0;

  if (strncmp(p->name, "19857", 50) == 0) digid = 0;
  if (strncmp(p->name, "19858", 50) == 0) digid = 1;
  if (strncmp(p->name, "24190", 50) == 0) digid = 2;
  if (strncmp(p->name, "24191", 50) == 0) digid = 3;
  if (strncmp(p->name, "24192", 50) == 0) digid = 4;
  if (strncmp(p->name, "24193", 50) == 0) digid = 5;
  if (strncmp(p->name, "24194", 50) == 0) digid = 6;
  if (strncmp(p->name, "24195", 50) == 0) digid = 7;
  if (strncmp(p->name, "24196", 50) == 0) digid = 8;
  if (strncmp(p->name, "24305", 50) == 0) digid = 9;
  if (strncmp(p->name, "24306", 50) == 0) digid = 10;
  if (strncmp(p->name, "24307", 50) == 0) digid = 11;
  if (strncmp(p->name, "24308", 50) == 0) digid = 12;
  if (strncmp(p->name, "24309", 50) == 0) digid = 13;
  if (strncmp(p->name, "24310", 50) == 0) digid = 14;
  if (strncmp(p->name, "24311", 50) == 0) digid = 15;
  if (strncmp(p->name, "24312", 50) == 0) digid = 16;

  for (uint16_t i=0; i<p->nEvents; i++) {
    uint64_t timetag = p->timetags[i];
    uint64_t exttimetag = p->exttimetags[i];
    uint64_t t = (exttimetag << 32) | timetag;

    if (offsets.caen[digid] == 0) {
      offsets.caen[digid] = t;
    }

    uint64_t key = daq_key(t - offsets.caen[digid], NULL);

    Event* e_prev = event_at(key-1);
    Event* e = event_at(key);
    Event* e_next = event_at(key+1);

    int e_mask = !(e_prev == NULL);
    e_mask |= !(e == NULL) << 1;
    e_mask |= !(e_next == NULL) << 2;

    if (!e_mask) {
      e = event_create(key);
      make_caenevent(i, p, &e->caen[digid]);
      e->caen_status |= (1 << digid);
    }
    else {
      if (e_mask == 0x1) {
        printf("# correcting CAEN key: %lu -> %lu\n", key, key-1);
        e = e_prev;
        key -= 1;
      }
      else if (e_mask == 0x4) {
        printf("# correcting CAEN key: %lu -> %lu\n", key, key+1);
        e = e_next;
        key += 1;
      }
      else if (e_mask != 0x2) {
        printf("# warning, found multiple keys. Key, mask: %lu, %i\n", key, e_mask);
        continue;
      }

      if (e->caen_status & (1 << digid)) {
        printf("# collision for caen, key %li!\n", key);
        continue;
      }

      pthread_mutex_lock(&e->lock);
      make_caenevent(i, p, &e->caen[digid]);
      e->caen_status |= (1 << digid);
      pthread_mutex_unlock(&e->lock);
    }

    if (event_ready(e)) {
      pthread_mutex_lock(&record_lock);
      record_push(key, DETECTOR_EVENT, (void*)e);
      pthread_mutex_unlock(&record_lock);
    }
  }
}

