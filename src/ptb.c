#include <stdio.h>
#include <evb/config.h>
#include <evb/ds.h>
#include <evb/listener.h>
#include <evb/ptb.h>

extern Config* config;
extern Event* events;
extern Record* records;
extern RunStart rs;

extern pthread_mutex_t record_lock;
extern time_offsets offsets;

uint64_t ptb_key(uint64_t timestamp, uint64_t* ts) {
  uint64_t t;
  if(config->evb_ptb_clk_scale == (uint64_t)config->evb_ptb_clk_scale) {
    t = timestamp * (uint64_t)config->evb_ptb_clk_scale;
  }
  else {
    t = timestamp * config->evb_ptb_clk_scale;
  }
  if (ts) *ts = t;
  return t / config->evb_slice;
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
        pthread_mutex_lock(&e->lock);
        memcpy((void*)(&e->ptb), (void*)t, sizeof(ptb_trigger_t));
        e->ptb_status = 1;
        pthread_mutex_unlock(&e->lock);
      }
      else {
        if(e_mask == 0x1) {
          e = e_prev;
          key -= 1;
        }
        else if(e_mask == 0x4){
          e = e_next;
          key += 1;
        }
        else if(e_mask != 0x2) {
          printf("# warning, found multiple keys. Key, mask: %lu, %i\n", key, e_mask);
          continue;
        }

        if (e->ptb_status) {
          if (rs.source_type == AMBE) {
            if(e->ptb.timestamp == t->timestamp - 2) {}
            else if (e->ptb.timestamp == t->timestamp + 2) continue;
            else {
              printf("# collision for ptb, key %li!\n", key);
              continue;
            }
          }
          else {
            printf("# collision for ptb, key %li!\n", key);
            continue;
          }
        }

        pthread_mutex_lock(&e->lock);  // what happens if a lock is destroyed while we wait?
        memcpy((void*)(&e->ptb), (void*)t, sizeof(ptb_trigger_t));
        e->ptb_status = 1;
        pthread_mutex_unlock(&e->lock);
      }

      if (event_ready(e)) {
        record_push(&records, key, DETECTOR_EVENT, (void*)e);
      }
    }
    else if (temp_word->word_type == ptb_t_fback) {}
    else if (temp_word->word_type == ptb_t_ch) {}
    else {}
  }
}

