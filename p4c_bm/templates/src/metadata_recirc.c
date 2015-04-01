/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdlib.h>
#include <string.h>

#include "enums.h"
#include "fields.h"
#include "metadata_recirc.h"

struct metadata_recirc_set_s {
  bool fields_valid[RMT_FIELD_INSTANCE_COUNT];
  int validity_count;
};

metadata_recirc_set_t *metadata_recirc_create(void) {
  metadata_recirc_set_t * set = malloc(sizeof(metadata_recirc_set_t));
  memset(set, 0, sizeof(metadata_recirc_set_t));
  return set;
}

void metadata_recirc_add_field(metadata_recirc_set_t *set,
			       rmt_field_instance_t field) {
  if(!set->fields_valid[field]) {
    set->fields_valid[field] = 1;
    set->validity_count++;
  }
}

void metadata_recirc_remove_field(metadata_recirc_set_t *set,
			          rmt_field_instance_t field) {
  if(set->fields_valid[field]) {
    set->fields_valid[field] = 0;
    set->validity_count--;
  }
}

void metadata_recirc_add_header(metadata_recirc_set_t *set,
				rmt_header_instance_t hdr) {
  rmt_field_instance_t field = rmt_header_instance_first_field(hdr);
  int i;
  for (i = 0; i < rmt_header_instance_num_fields(hdr); i++) {
    if(!set->fields_valid[field + i]) {
      set->fields_valid[field + i] = 1;
      set->validity_count++;
    }
  }
}

int *metadata_recirc_digest(metadata_recirc_set_t *set) {
  int *res = malloc(sizeof(int) * (set->validity_count + 1));
  res[0] = set->validity_count;
  int field;
  int pos = 1;
  for (field = 0; field < RMT_FIELD_INSTANCE_COUNT; field++) {
    if(set->fields_valid[field]) {
      res[pos++] = field;
    }
  }
  return res;
}


metadata_recirc_set_t *metadata_recirc_init(int *data) {
  metadata_recirc_set_t *set = metadata_recirc_create();
  set->validity_count = data[0];
  int i;
  for(i = 1; i < (data[0] + 1); i++) {
    set->fields_valid[data[i]] = 1;
  }
  return set;
}

void metadata_recirc_empty(metadata_recirc_set_t *set) {
  memset(set, 0, sizeof(metadata_recirc_set_t));
}

int metadata_recirc_is_valid(metadata_recirc_set_t *set,
			     rmt_field_instance_t field) {
  return set->fields_valid[field];
}
