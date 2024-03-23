#ifndef __PENNBUILDER_LISTENER__
#define __PENNBUILDER_LISTENER__

#define MAX_BUFFER_LEN 350000

typedef struct {
  uint16_t packet_type;
  uint16_t len;
} NetMeta;

void close_sockets();
void handler(int signal);
void die(const char *msg);
void* listener_child(void* psock);
void* listener(void* ptr);

typedef enum {
  CTL_PACKET,
  CMD_PACKET,
  PTB_PACKET,
  DAQ_PACKET,

  // Old...
  XL3_PACKET,
  MTC_PACKET,
  CAEN_PACKET,
  TRIG_PACKET,
  EPED_PACKET,
  RHDR_PACKET,
  CAST_PACKET,
  CAAC_PACKET
} PacketType;

typedef struct {
  uint16_t type;
} PacketHeader;

#endif

/*
typedef struct
{
  uint16_t packet_num;
  uint8_t packet_type;
  uint8_t num_bundles;
} XL3_CommandHeader;

typedef struct
{
  PacketHeader header;
  XL3_CommandHeader cmdHeader;
  char payload[XL3_MAXPAYLOADSIZE_BYTES];
} XL3Packet;
*/

