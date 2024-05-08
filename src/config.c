#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <yaml.h>
#include <evb/config.h>
#include <json-c/json.h>

json_object* get_obj(json_object* o, char* section, char* field) {
  json_object *o_s, *o_f;
  assert((o_s = json_object_object_get(o, section)));
  assert((o_f = json_object_object_get(o_s, field)));
  return o_f;
}

Config* config_parse(char* config_file) {
  Config* config = malloc(sizeof(Config));
  config_file = "./config/rutgers_vst.json";
  config->file = config_file;

  json_object* json = json_object_from_file(config_file);
  assert(json);

  // Builder
  config->evb_slice = json_object_get_double(get_obj(json, "builder", "slice"));

  config->evb_ptb_clk_scale = \
    json_object_get_double(get_obj(json, "builder", "ptb_clk"));

  config->evb_port = json_object_get_double(get_obj(json, "network", "port"));

  // Monitor
  config->monitor_address = \
    (char*)json_object_get_string(get_obj(json, "monitor", "address"));

  config->monitor_port = \
    json_object_get_int(get_obj(json, "monitor", "port"));

  // DAQ
  json_object* digitizers = get_obj(json, "daq", "digitizers");
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

