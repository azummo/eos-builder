#ifndef __PENNBUILDER_SHIPPER__
#define __PENNBUILDER_SHIPPER__

#define MAX_RHDR_WAIT 100000*50

// CDABHeader: a header that precedes each structure in a CDAB file
typedef struct {
  uint32_t record_type;
  uint32_t size;
} CDABHeader;


// FIFO based on https://stackoverflow.com/questions/215557
#define WRITE_QUEUE_ELEMENTS 100000
#define WRITE_QUEUE_SIZE (WRITE_QUEUE_ELEMENTS + 1)
uint64_t write_queue[WRITE_QUEUE_SIZE];
uint64_t write_head, write_tail;

void queue_init();
uint64_t queue_push(uint64_t x);
uint64_t queue_pop(uint64_t *x);

// Shipper
void* shipper(void* ptr);

#endif

