#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <yaml.h>
#include <evb/config.h>

#include <json-c/json.h>

Config* config_parse(char* config_file) {
  Config* config = malloc(sizeof(Config));
  config_file = "./config/rutgers_vst.json";
  config->file = config_file;

  json_object* json = json_object_from_file(config_file);
  assert(json);

  // Builder
  json_object* builder = json_object_object_get(json, "builder");
  assert(builder);

  json_object* slice = json_object_object_get(builder, "slice");
  assert(slice);
  config->evb_slice = json_object_get_int(slice);

  json_object* clk = json_object_object_get(builder, "ptb_clk");
  assert(clk);
  config->evb_ptb_clk_scale = json_object_get_double(clk);

  // Network
  json_object* network = json_object_object_get(json, "network");
  assert(network);

  json_object* port = json_object_object_get(network, "port");
  assert(port);

  config->evb_port = json_object_get_int(port);

  // Monitor
  json_object* monitor = json_object_object_get(json, "monitor");
  assert(monitor);

  json_object* address = json_object_object_get(monitor, "address");
  assert(address);

  config->monitor_address = (char*)json_object_get_string(address);

  port = json_object_object_get(monitor, "port");
  assert(port);

  config->monitor_port = json_object_get_int(port);

  // DAQ
  json_object* daq = json_object_object_get(json, "daq");
  assert(daq);

  json_object* digitizers = json_object_object_get(daq, "digitizers");
  assert(digitizers);

  config->dig_ndig = json_object_array_length(digitizers);
  config->dig_mask = 0;
  for (int i=0; i<config->dig_ndig; i++) {
    json_object* serial = json_object_array_get_idx(digitizers, i);
    config->dig_ids[i] = (char*) json_object_get_string(serial);
    if (strncmp(config->dig_ids[i], "", 100) != 0) {
      config->dig_mask |= 1<<i;
    }
  }

  return config;
}

void config_print(const Config* c) {
  printf("* CONFIG: %s\n", c ? c->file : "NONE");
  if (!c) return;

  printf("*  evb_port: %i\n", c->evb_port);
  printf("*  monitor_address: %s\n", c->monitor_address);
  printf("*  monitor_port: %i\n", c->monitor_port);
  printf("*  dig_ndig: %i\n", c->dig_ndig);
  printf("*  dig_mask: %x\n", c->dig_mask);
  printf("*  dig_ids: [");
  for (int i=0; i<c->dig_ndig; i++) {
    printf(" %s ", c->dig_ids[i]);
  }
  printf("]\n");
}

