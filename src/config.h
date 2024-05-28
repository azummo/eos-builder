#ifndef __EOSBUILDER_CONFIG__
#define __EOSBUILDER_CONFIG__

#include <stdint.h>

#define MAX_DIGITIZERS 100

typedef struct {
  char* file;  //!< Config file

  char* output_dir; //!< Output directory
  double max_file_size; //!< Max file size in gigabytes before creating new subrun

  int evb_port;  //!< Event builder port
  int evb_slice;  //!< Timestamp chunking
  float evb_ptb_clk_scale;  //!< PTB vs. CAEN clock scale

  char* monitor_address;  //!< Monitor address
  int monitor_port;  //!< Monitor port

  int ptb_exists;  //!< If the ptb exists
  uint8_t dig_ndig;  //!< Number of digitizers
  uint32_t dig_mask;  //!< Bit mask for expected digitizers
  char* dig_ids[MAX_DIGITIZERS];  //!< Digitizer ID serial names
} Config;

Config* config_parse(char* config_file);

void config_print(const Config* c);

#endif

