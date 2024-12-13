#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <jemalloc/jemalloc.h>
#include <evb/config.h>
#include <evb/shipper.h>
#include <evb/ds.h>
#include <evb/version.h>

extern Config* config;
extern Event* events;
extern Record* records;
extern Record* headers;
extern pthread_mutex_t record_lock;

uint32_t bytes_written;
double file_gigabytes_written;
uint32_t events_written;

int ninactive;

void handler(int signal);

char fileid[250] = "";
char filename[270] = "";
char convert_command[500] = "";
FILE* outfile;

RunStart rs;
RunEnd re;

void send_all(int socket_handle, char* data, int size) {
  int total = 0;
  int bytesleft = size;
  int n;

  while (total < size) {
    n = send(socket_handle, data+total, bytesleft, 0);
    if (n == -1) {
      perror("ERROR: send_all");
      break;
    }
    total += n;
    bytesleft -= n;
  }

  assert(total == size);
}

void *convert_function(void *arg) {
    char *command = (char *)arg;
    system(command);
    return NULL;
}

void convert_cdab(char* fileid, int last_subrun_number) {
 pthread_t convert_thread_id;
 sprintf(convert_command,"%s %s_%03i.cdab %s_%03i.hdf5", config->converter,
         fileid, last_subrun_number, fileid, last_subrun_number);
 int result = pthread_create(&convert_thread_id, NULL, convert_function, convert_command);
 if (result != 0) {
   perror("pthread_create failed");
 }
  pthread_detach(convert_thread_id);
}

void* shipper(void* ptr) {
  outfile = NULL;
  //int run_id = -1;
  //int start_key = 0;

  //sprintf(filename, "run_%i.cdab", 0);
  //printf("> Run %i, key %i => %s\n", run_id, start_key, filename);
  //outfile = fopen(filename, "wb+");

  // Connect to a monitor
  struct addrinfo hints, *res;
  int sockfd = -1;
  if (strcmp(config->monitor_address, "") != 0) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char portno[50];
    snprintf(portno, 50, "%i", config->monitor_port);
    assert(getaddrinfo(config->monitor_address, portno, &hints, &res) == 0);
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    assert(sockfd != -1);
    int r = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (r == -1) {
      printf("# error connecting to monitor %s:%i\n",
             config->monitor_address, config->monitor_port);
    }
    assert(r != -1);
    printf("* connected to monitor %s:%i\n",
           config->monitor_address, config->monitor_port);
  }

  signal(SIGINT, &handler);
  int run_number = 0;
  int subrun_number = 0;
  Record* r;
  bool end_run = false;
  uint64_t h_key = -1;
  uint64_t e_key = -1;

  while (1) {
    if(!end_run) h_key = record_next(&headers);
    e_key = record_next(&records);

    // Handle run start/stop
    if (h_key != -1) {
      if(!end_run) r = record_pop(&headers, h_key);

      if (r && r->type == RUN_START && h_key <= e_key) {
        RunStart* run_start = (RunStart*) r->data;
        rs = *run_start;
        rs.version_major = VERSION_MAJOR;
        rs.version_minor = VERSION_MINOR;
        rs.version_patch = VERSION_PATCH;
        run_number = rs.run_number;
        subrun_number = 0;

        if (outfile) {
          printf("# new run %i started with run active.\n", run_number);
          fclose(outfile);
          outfile = NULL;
        }
        struct stat sb;
        if (stat(rs.outdir, &sb) == 0 && S_ISDIR(sb.st_mode)){
          sprintf(fileid, "%seos_run_%06i", rs.outdir, run_number);
        }
	else {
          printf("Output directory %s does not exist\n", rs.outdir);
	  printf("Outputting to current directory\n");
          sprintf(fileid, "eos_run_%06i", run_number);
        }
        sprintf(filename, "%s_%03i.cdab", fileid, subrun_number);
        printf("> Start run %i, key %li => %s\n", run_number, h_key, filename);
        outfile = fopen(filename, "wb+");

        CDABHeader cdh;
        cdh.record_type = RUN_START;
        cdh.size = sizeof(RunStart);
        fwrite(&cdh, sizeof(CDABHeader), 1, outfile);
        fwrite(&rs, sizeof(RunStart), 1, outfile);
      }
      else if (r && r->type == RUN_END && h_key >= e_key) {
        end_run = true;
      }
      else if (r && r->type == RUN_END && (h_key < e_key || e_key == -1)) {
        RunEnd* run_end = (RunEnd*) r->data;
        re = *run_end;
        if (outfile) {

          CDABHeader cdh;
          cdh.record_type = RUN_END;
          cdh.size = sizeof(RunEnd);
          fwrite(&cdh, sizeof(CDABHeader), 1, outfile);
          fwrite(&re, sizeof(RunEnd), 1, outfile);

          printf("< End run %i, key %li => %s\n", run_number, h_key, filename);
          fclose(outfile);
          outfile = NULL;
          sprintf(filename,"NO RUN ACTIVE");
          printf("> Processing subrun %i\n", subrun_number);
          convert_cdab(fileid, subrun_number);
	  end_run = false;
        }
      }
    }

    if (e_key == -1) {
      usleep(100);
      continue;
    }

    Record* r = record_pop(&records, e_key);

    if (!r) {
      printf("popped null Record pointer!? key=%li\n", e_key);
      continue;
    }

    if (r->type == DETECTOR_EVENT) {
      Event* e = event_pop(e_key);

      if (!e) {
        printf("popped null Event pointer!? key=%li\n", e_key);
        continue;
      }

    if (!outfile) {
      ninactive++;
/*
      printf("# events received with no run active, key %li.\n", e_key);
      uuid_t uuid;
      char suuid[400];
      uuid_generate(uuid);
      uuid_unparse(uuid, suuid);
      snprintf(filename, 500, "default_%s.cdab", suuid);
      printf("> Default run %s, key %li => %s\n", uuid, e_key, filename);
*/
      free(e);
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
      bytes_written += sizeof(Event) + sizeof(CDABHeader);
      file_gigabytes_written += (double)(sizeof(Event) + sizeof(CDABHeader))/1e9;
      events_written++;

      if (sockfd > -1) {
        send_all(sockfd, (char*) &cdh, sizeof(CDABHeader));
        send_all(sockfd, (char*) e, sizeof(Event));
      }

      free(e);

      if (file_gigabytes_written > config->max_file_size) {
        int last_subrun_number = subrun_number;
        subrun_number++;
        sprintf(filename, "%s_%03i.cdab", fileid, subrun_number);
	fclose(outfile);
        outfile = fopen(filename, "wb+");

        CDABHeader cdh;
        cdh.record_type = RUN_START;
        cdh.size = sizeof(RunStart);
        fwrite(&cdh, sizeof(CDABHeader), 1, outfile);
        fwrite(&rs, sizeof(RunStart), 1, outfile);

	printf("> Start subrun %i => %s\n", subrun_number, filename);
        file_gigabytes_written = 0;

        printf("> Processing subrun %i\n", last_subrun_number);
        convert_cdab(fileid, last_subrun_number);
      }
    }

    else {
      printf("# popped unknown record type %i\n", r->type);
    }
  }
}

