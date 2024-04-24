#include <stdio.h>
#include <evb/ds.h>
#include <evb/listener.h>
#include <evb/ptb.h>

extern Event* events;
extern Record* records;

extern pthread_mutex_t record_lock;
extern time_offsets offsets;


uint64_t ptb_key(uint64_t timestamp, uint64_t* ts) {
  uint64_t t = timestamp * 2;
  if (ts) *ts = t;
  return t / 4;
}


void accept_ptb(char* data) {
  const size_t header_size = sizeof(ptb_tcp_header_t);
  const size_t word_size = 4 * sizeof(uint32_t);

  ptb_tcp_header_t* head = (ptb_tcp_header_t*) data;

  int n_bytes = head->packet_size;
  int n_words = n_bytes / word_size;

  for (int i=0; i<n_words; i++) {
    ptb_word_t* temp_word = (ptb_word_t*) (data+header_size+i*word_size);

    if (temp_word->word_type == ptb_t_ts) {}
    else if (temp_word->word_type == ptb_t_lt) {}
    else if (temp_word->word_type == ptb_t_gt) {
      ptb_trigger_t* t = (ptb_trigger_t*) temp_word;

      if (offsets.ptb == 0) {
        offsets.ptb = t->timestamp;
      }

      if (t->timestamp < offsets.ptb_last) {
        offsets.ptb_dt += 1 << 27;  // ???
      }
      offsets.ptb_last = t->timestamp;

      uint64_t ts = t->timestamp + offsets.ptb_dt - offsets.ptb;

      uint64_t key = ptb_key(ts, NULL);

      Event* e_prev = event_at(key-1);
      Event* e = event_at(key);
      Event* e_next = event_at(key+1);

      int e_mask = !(e_prev == NULL);
      e_mask |= !(e == NULL) << 1;
      e_mask |= !(e_next == NULL) << 2;

      if (!e_mask) {
        e = event_create(key);
        memcpy((void*)(&e->ptb), (void*)t, sizeof(ptb_trigger_t));
        e->ptb_status = 1;
      }
      else {
        if(e_mask == 0x1) {
          printf("# correcting PTB key: %lu -> %lu\n", key, key-1);
          e = e_prev;
          key -= 1;
        }
        else if(e_mask == 0x4){
          printf("# correcting PTB key: %lu -> %lu\n", key, key+1);
          e = e_next;
          key += 1;
        }
        else if(e_mask != 0x2) {
          printf("# warning, found multiple keys. Key, mask: %lu, %i\n", key, e_mask);
          continue;
        }

        if (e->ptb_status) {
          printf("# collision for ptb, key %li!\n", key);
          continue;
        }

        pthread_mutex_lock(&e->lock);  // what happens if a lock is destroyed while we wait?
        memcpy((void*)(&e->ptb), (void*)t, sizeof(ptb_trigger_t));
        e->ptb_status = 1;
        pthread_mutex_unlock(&e->lock);
      }

      if (event_ready(e)) {
        pthread_mutex_lock(&record_lock);
        record_push(key, DETECTOR_EVENT, (void*)e);
        pthread_mutex_unlock(&record_lock);
      }
    }
    else if (temp_word->word_type == ptb_t_fback) {}
    else if (temp_word->word_type == ptb_t_ch) {}
    else {}
  }
}

