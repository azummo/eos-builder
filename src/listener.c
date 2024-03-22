#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <jemalloc/jemalloc.h>
#include <evb/listener.h>
#include <evb/ds.h>
#include <evb/ptb.h>
#include <evb/uthash.h>

#define NUM_THREADS 5

//extern Buffer* event_buffer;
//extern Buffer* event_header_buffer;
//extern Buffer* run_header_buffer;
extern FILE* outfile;

int sockfd, thread_sockfd[NUM_THREADS];

void die(const char *msg)
{
  perror(msg);
  exit(1);
}

void close_sockets()
{
  int i;
  for(i=0; i<NUM_THREADS; i++)
    close(thread_sockfd[i]);
  close(sockfd);
}

void handler(int signal)
{
  if(signal == SIGINT) {
    printf("\nCaught CTRL-C (SIGINT), Exiting...\n");
    //if(!buffer_isempty(event_buffer)) {
    //    printf("Warning: exiting with non-empty event buffer\n");
    //    buffer_status(event_buffer);
    //}
    //if(!buffer_isempty(event_header_buffer)) {
    //    printf("Warning: exiting with non-empty event header buffer\n");
    //    buffer_status(event_header_buffer);
    //}
    //if(!buffer_isempty(run_header_buffer)) {
    //    printf("Warning: exiting with non-empty run header buffer\n");
    //    buffer_status(run_header_buffer);
    //}
    printf("Closing sockets...\n");
    close_sockets();
    if (outfile) {
      printf("Closing run file...\n");
      fclose(outfile);
    }
    exit(0);
  }
  else
    printf("\nCaught signal %i\n", signal);
}

void recv_all(int socket_handle, char* dest, int size) {
  int total = 0;
  int bytesleft = size;
  int n;

  while (total < size) {
    n = recv(socket_handle, dest+total, bytesleft, 0);
    if (n == -1) {
      perror("ERROR: recv_all");
      break;
    }
    total += n;
    bytesleft -= n;
  }

  assert(total == size);
}

void accept_ptb(char* data) {
  const size_t header_size = sizeof(ptb_tcp_header_t);
  const size_t word_size = 4 * sizeof(uint32_t);

  ptb_tcp_header_t* head = (ptb_tcp_header_t*) data;

  int n_bytes = head->packet_size;
  int n_words = n_bytes / word_size;

  for (int i=0; i<n_words; i++) {
    ptb_word_t* temp_word = (ptb_word_t*) (data+header_size+i*word_size);

    if (temp_word->word_type == ptb_t_ts) {
      //printf("TS\n");
    }
    else if (temp_word->word_type == ptb_t_lt) {
      ptb_trigger_t* t = (ptb_trigger_t*) temp_word;
      //printf("LLT / type: %04x, timestamp: %04lx, word: %04lx\n",
      //       t->word_type, t->timestamp, (uint64_t) t->trigger_word);
    }
    else if (temp_word->word_type == ptb_t_gt) {
      ptb_trigger_t* t = (ptb_trigger_t*) temp_word;

      printf("HLT / type: %04x, timestamp: %04lx, word: %04lx\n",
             t->word_type, t->timestamp, (uint64_t) t->trigger_word);
    }
    else if (temp_word->word_type == ptb_t_fback) {
      //printf("FEEDBACK\n");
      //ptb::content::word::feedback_t* feedback = \
      //  (ptb::content::word::feedback_t*) (&temp_word);
    }
    else if (temp_word->word_type == ptb_t_ch) {
      //printf("CH STATUS\n");
    }
    else {
      //printf("Word: OTHER, type = %i\n", temp_word->word_type);
    }
  }
  
}

void accept_daq(char* packet_buffer)
{
  DigitizerData* p = (DigitizerData*) (packet_buffer+4);

  uint8_t digid = 0; //p->digitizer_id;

  for (uint16_t i=0; i<p->nEvents; i++) {
    uint32_t timetag = p->timetags[i];

    int key = (int) p->timetags[i] / 100;
    Event* e = event_at(key);

    if (!e) {
      printf("DAQ / time: %04x\n", timetag);
      //printf("DAQ: create new event, key=%i\n", key);
      e = (Event*) malloc(sizeof(Event));
      e->id = key;
      e->timetag = p->timetags[i];
      e->datatype = 0;
      e->mcflag = 0;
      e->caen_status = 0;  // who has data?
      //e->run_id = ?
      //e->subrun_id = ?
      //e->nhits = ?

      clock_t t = clock();
      e->builder_arrival_time = t;

      memcpy((void*)(&e->caen[digid]), (void*) p, sizeof(DigitizerData));
      e->caen_status |= (1 << digid);

      event_push(key, e);
    }
    else {
      memcpy((void*)(&e->caen[digid]), (void*) p, sizeof(DigitizerData));
      e->caen_status |= (1 << digid);
      //printf("DAQ: append to existing event, key=%i\n", key);
    }
  }
}

void* listener_child(void* psock)
{
  int sock = *((int*) psock);
  char* packet_buffer = malloc(MAX_BUFFER_LEN);
  bzero(packet_buffer, MAX_BUFFER_LEN);

  signal(SIGINT, &handler);
  while(1) {
    int r = recv(sock, packet_buffer, sizeof(NetMeta), 0);
    if (r<0) {
      die("Error reading from socket");
      return NULL;
    }
    else if (r==0) {
      printf("Client terminated connection on socket %i\n", sock);
      return NULL;
    }

    NetMeta* meta = (NetMeta*) packet_buffer;
    PacketType packet_type = meta->packet_type;
    uint16_t packet_size = meta->len;

    // Handle packet types
    if (((ptb_tcp_header_t*) packet_buffer)->format_version == 45) {
      ptb_tcp_header_t* head = (ptb_tcp_header_t*) packet_buffer;
      //printf("PTB:HEAD / packet_size: %i, sequence_id: %i, format_version: %i\n",
      //head->packet_size, head->sequence_id, head->format_version);

      const size_t header_size = sizeof(ptb_tcp_header_t);
      const size_t word_size = 4 * sizeof(uint32_t);
      int n_words = head->packet_size / word_size;
      //printf("PTB word_size=%i, packet_size=%i, n_words=%i\n", word_size, head->packet_size, n_words);
      for (int i=0; i<n_words; i++) {
        recv_all(sock, packet_buffer+header_size+i*word_size, word_size);
      }

      accept_ptb(packet_buffer);
    }

    else if(packet_type == DAQ_PACKET) {
      //printf("recv DAQ_PACKET\n");
      //memset(packet_buffer+4, 0, sizeof(DigitizerData));

      //printf("DAQ: type=%i\n", (int)packet_type);

      recv_all(sock, packet_buffer+4, sizeof(DigitizerData));

      accept_daq(packet_buffer);
    }

    //else if(packet_type == RHDR_PACKET)
    //    continue;
    //else if(packet_type == CAST_PACKET)
    //    continue;
    //else if(packet_type == CAAC_PACKET)
    //    continue;

    else {
      printf("Unknown packet type %u on socket %i\n", packet_type, sock);
      char ss[50];
      snprintf(ss, 50, "%s", (char*)packet_buffer);
      printf("Content: %s\n", ss);
      // do something
      break;
    }
  }

  return NULL;
}


void* listener(void* ptr)
{
  int portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  portno = *((int*)ptr);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0) die("ERROR opening socket");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    die("ERROR on binding");

  listen(sockfd, 5);

  clilen = sizeof(cli_addr);
  signal(SIGINT, &handler);
  pthread_t threads[NUM_THREADS];
  int thread_index = 0;
  while(1) {
    int newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
    if(newsockfd < 0) die("ERROR on accept");
    else {
      thread_sockfd[thread_index] = newsockfd;
      printf("Spawning listener_child thread with index %i\n", thread_index);
      pthread_create(&(threads[thread_index]),
            NULL,
            listener_child,
            (void*)&(thread_sockfd[thread_index]));
      thread_index++;
    }
  }

  close_sockets();
}

