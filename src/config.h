#ifndef __EOSBUILDER_CONFIG__
#define __EOSBUILDER_CONFIG__

#include <stdint.h>

#define MAX_DIGITIZERS 100

typedef struct {
  char* file;  //!< Config file

  int evb_port;  //!< Event builder port

  char* monitor_address;  //!< Monitor address
  int monitor_port;  //!< Monitor port

  uint8_t dig_ndig;  //!< Number of digitizers
  uint32_t dig_mask;  //!< Bit mask for expected digitizers
  char* dig_ids[MAX_DIGITIZERS];  //!< Digitizer ID serial names
} Config;

Config* config_parse(char* config_file);

void config_print(const Config* c);

#endif

