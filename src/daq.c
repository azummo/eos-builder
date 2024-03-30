#include <stdio.h>
#include <evb/ds.h>
#include <evb/listener.h>
#include <evb/daq.h>

extern pthread_mutex_t record_lock;
extern time_offsets offsets;


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
      memcpy((void*)(&e->caen[digid]), (void*)p, sizeof(DigitizerData));
      e->caen_status |= (1 << digid);
    }
    else {
      if (e->caen_status & (1 << digid)) {
        printf("# collision for caen, key %li!\n", key);
        continue;
      }

      pthread_mutex_lock(&e->lock);
      memcpy((void*)(&e->caen[digid]), (void*)p, sizeof(DigitizerData));
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

