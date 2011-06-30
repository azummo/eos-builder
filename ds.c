#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <jemalloc/jemalloc.h>
#include "ds.h"

uint32_t get_bits(uint32_t x, uint32_t position, uint32_t count)
{
  uint32_t shifted = x >> position;
  uint32_t mask = ((uint64_t)1 << count) - 1;
  return shifted & mask;
}

void pmtbundle_print(PMTBundle* p)
{
    printf("PMTBundle at %p:\n", p);
    printf("  pmtid =  %i:\n", pmtbundle_pmtid(p));
    printf("  gtid  =  %i:\n", pmtbundle_gtid(p));
    int i;
    for(i=0; i<3; i++)
        printf("  word%i =  %u:\n", i, p->word[i]);
}

uint32_t pmtbundle_pmtid(PMTBundle* p)
{
    int ichan = get_bits(p->word[0], 16, 5);
    int icard = get_bits(p->word[0], 26, 4);
    int icrate = get_bits(p->word[0], 21, 5);
    return (512*icrate + 32*icard + ichan);
}

uint32_t pmtbundle_gtid(PMTBundle* p)
{
    uint32_t gtid1 = get_bits(p->word[0], 0, 16);
    uint32_t gtid2 = get_bits(p->word[2], 12, 4);
    uint32_t gtid3 = get_bits(p->word[2], 28, 4);
    return (gtid1 + (gtid2<<16) + (gtid3<<20));
}

Buffer* buffer_alloc(Buffer** pb, int size)
{
    *pb = malloc(sizeof(Buffer));
    (*pb)->keys = malloc(size * sizeof(void*));
    (*pb)->type = malloc(size * sizeof(RecordType));
    int mem_allocated = sizeof(Buffer) + size * (sizeof(void*) + sizeof(RecordType));
    if(*pb) {
        printf("Initializing buffer: keys[%d] (%dKB allocated)\n", size, mem_allocated/1000);
        bzero(*pb, sizeof(*pb));
        (*pb)->size = size;
        (*pb)->end = 0;
        (*pb)->start  = 0;
        (*pb)->offset = 0;
        pthread_mutex_init(&((*pb)->mutex), NULL);
    }
    return *pb;
}
 
int buffer_isfull(Buffer* b)
{
    return (((b->end + 1) % b->size) == b->start);
}
 
int buffer_isempty(Buffer* b)
{
    return (b->start == b->end);
}
 
int buffer_push(Buffer* b, RecordType type, void* key)
{
    pthread_mutex_lock(&(b->mutex));
    int full = buffer_isfull(b);
    if(!full)
    {
        b->keys[b->end] = key;
        b->type[b->end] = type;
        b->end++;
        b->end %= b->size;
    }
    pthread_mutex_unlock(&(b->mutex));
    return !full;
}

int buffer_pop(Buffer* b, RecordType* type, void** pk)
{
    pthread_mutex_lock(&(b->mutex));
    int empty = buffer_isempty(b);
    if(!empty)
    {
        (*pk) = b->keys[b->start];
        (*type) = b->type[b->start];
        b->keys[b->start] = NULL;
        b->type[b->start] = 0;
        b->start++; // note: you can pop a NULL pointer off the end
        b->start %= b->size;
    }
    pthread_mutex_unlock(&(b->mutex));
    return !empty;
}

void buffer_status(Buffer* b)
{
    printf("Buffer at %p:\n", b);
    printf("  write: %lu, read: %lu, full: %d, empty: %d\n", b->end,
                                                             b->start,
                                                             buffer_isfull(b),
                                                             buffer_isempty(b));
}

void buffer_clear(Buffer* b)
{
    pthread_mutex_lock(&(b->mutex));
    int i;
    for(i=0; i<b->size; i++) {
        if(b->keys[i] != NULL)
            free(b->keys[i]);
        b->keys[i] = NULL;
        b->type[i] = 0;
    }
    b->end = 0;
    b->start = 0;
    pthread_mutex_unlock(&(b->mutex));
}

int buffer_at(Buffer* b, unsigned int id, RecordType* type, void** pk)
{
    int keyid = (id - b->offset) % b->size;
    if (keyid < b->size) {
        *type = b->type[keyid];
        *pk = b->keys[keyid];
        return pk == NULL ? 1 : 0;
    }
    else
        return 1;
}

int buffer_insert(Buffer* b, unsigned int id, RecordType type, void* pk)
{
    pthread_mutex_lock(&(b->mutex));
    int keyid = (id - b->offset) % b->size;
    if(keyid < b->size) {
        b->type[keyid] = type;
        b->keys[keyid] = pk;
        if(keyid > b->end) {
            b->end = keyid;
            b->end %= b->size;
        }
        if(keyid < b->start) {
            printf("buffer_insert: got record with id %i < read position %lu\n", keyid, b->start);
            // received data for already-shipped event, do something
        }
        pthread_mutex_unlock(&(b->mutex));
        return 0;
    }
    else {
        pthread_mutex_unlock(&(b->mutex));
        return 1;
    }
}

