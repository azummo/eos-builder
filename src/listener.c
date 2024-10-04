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
#include <evb/config.h>
#include <evb/listener.h>
#include <evb/shipper.h>
#include <evb/ds.h>
#include <evb/ptb.h>
#include <evb/daq.h>
#include <evb/uthash.h>

#define NUM_THREADS 5

extern FILE* outfile;
extern Config* config;
extern Event* events;
extern Record* records;
extern Record* headers;

int sockfd, thread_sockfd[NUM_THREADS];

time_offsets offsets;

void die(const char* msg) {
  perror(msg);
  exit(1);
}

void close_sockets() {
  int i;
  for(i=0; i<NUM_THREADS; i++)
    if (thread_sockfd[i]) close(thread_sockfd[i]);
  close(sockfd);
}

void handler(int signal) {
  if(signal == SIGINT) {
    printf("\nCaught CTRL-C (SIGINT), Exiting...\n");
    if (event_count()) {
      printf("Warning: exiting with non-empty event buffer\n");
    }
    if (record_count(&records)) {
      printf("Warning: exiting with non-empty output buffer\n");
    }
    if (record_count(&headers)) {
      printf("Warning: exiting with non-empty header buffer\n");
    }
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

PacketType packet_id(char* h) {
  tcp_header* hdr = (tcp_header*) h;
  if (((ptb_tcp_header_t*) hdr)->format_version == 45) {
    return PTB_PACKET;
  }
  else {
    return (PacketType) hdr->type;
  }

  return UNK_PACKET;
}

void* listener_child(void* psock) {
  int sock = *((int*) psock);
  char* packet_buffer = malloc(MAX_BUFFER_LEN);
  bzero(packet_buffer, MAX_BUFFER_LEN);

  signal(SIGINT, &handler);

  while(1) {
    int r = recv(sock, packet_buffer, sizeof(tcp_header), 0);
    if (r<0) {
      die("Error reading from socket");
      return NULL;
    }
    else if (r==0) {
      printf("Client terminated connection on socket %i\n", sock);
      return NULL;
    }

    // Handle packet types
    PacketType type = packet_id(packet_buffer);

    switch (type) {
      case (PTB_PACKET): {
        ptb_tcp_header_t* head = (ptb_tcp_header_t*) packet_buffer;
        const size_t header_size = sizeof(ptb_tcp_header_t);
        const size_t word_size = 4 * sizeof(uint32_t);
        int n_words = head->packet_size / word_size;

        for (int i=0; i<n_words; i++) {
          recv_all(sock, packet_buffer+header_size+i*word_size, word_size);
        }

        accept_ptb(packet_buffer);
        break;
      }

      case (DAQ_PACKET): {
        recv_all(sock, packet_buffer+4, sizeof(DigitizerData));
        accept_daq(packet_buffer);
        break;
      }

      case (RUN_START_PACKET): {
        recv_all(sock, packet_buffer+4, sizeof(RunStart));
        accept_run_start(packet_buffer);
        break;
      }

      case (RUN_END_PACKET): {
        recv_all(sock, packet_buffer+4, sizeof(RunEnd));
        accept_run_end(packet_buffer);
        break;
      }

      default:
        printf("Unknown packet type %u on socket %i\n", (int)type, sock);
        char ss[50];
        snprintf(ss, 50, "%s", (char*)packet_buffer);
        printf("Content: %s\n", ss);
        break;
    }
  }

  return NULL;
}


void* listener(void* ptr) {
  int portno = *((int*) ptr);
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  memset(&offsets, 0, sizeof(time_offsets));
  memset(&thread_sockfd, 0, sizeof(thread_sockfd));

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
      memset(&offsets, 0, sizeof(time_offsets));
      pthread_create(&(threads[thread_index]),
            NULL,
            listener_child,
            (void*)&(thread_sockfd[thread_index]));
      thread_index++;
    }
  }

  close_sockets();
}

