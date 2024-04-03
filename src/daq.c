#include <stdio.h>
#include <evb/ds.h>
#include <evb/daq.h>
#include <evb/listener.h>

extern pthread_mutex_t record_lock;
extern time_offsets offsets;

CAENEvent* make_caenevent(int i, DigitizerData* caen, CAENEvent* e) {
  if (e == NULL) {
    e = (CAENEvent*) malloc(sizeof(CAENEvent));
  }

  e->type = caen->type;
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
  return t / 100000;
}

void accept_daq(char* data) {
  DigitizerData* p = (DigitizerData*) (data+4);
  uint8_t digid = 0; //p->digitizer_id;  // FIXME in WbLSdaq

  for (uint16_t i=0; i<p->nEvents; i++) {
    uint64_t timetag = p->timetags[i];
    uint64_t exttimetag = p->exttimetags[i];
    uint64_t t = (exttimetag << 32) | timetag;

    if (offsets.caen[digid] == 0) {
      offsets.caen[digid] = t;
    }

    uint64_t key = daq_key(t - offsets.caen[digid], NULL);

    Event* e = event_at(key);

    if (!e) {
      e = event_create(key);
      make_caenevent(i, p, &e->caen[digid]);
      e->caen_status |= (1 << digid);
    }
    else {
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

