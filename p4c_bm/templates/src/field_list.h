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

#include <stdint.h>

#include "enums.h"
#include "phv.h"

typedef enum field_list_entry_type_e {
  FIELD_LIST_ENTRY_FIELD,
  FIELD_LIST_ENTRY_HEADER,
  FIELD_LIST_ENTRY_UINT32,
  FIELD_LIST_ENTRY_BYTE_BUF,
  FIELD_LIST_ENTRY_FIELD_LIST,
  FIELD_LIST_ENTRY_PAYLOAD
} field_list_entry_type_t;

typedef struct field_list_s field_list_t;

typedef struct field_list_entry_s {
  field_list_entry_type_t type;
  union {
    rmt_header_instance_t header;
    rmt_field_instance_t field;
    uint32_t uint32_value;
    byte_buf_t byte_buf_value;
    field_list_t *field_list;
    void *payload;
  } data;
  int size;
} field_list_entry_t;

struct field_list_s {
  int size;
  field_list_entry_t *entries;
};

extern field_list_t _rmt_field_lists[RMT_FIELD_LIST_COUNT + 1];

static inline field_list_t *field_list_get(rmt_field_list_t field_list) {
  return &_rmt_field_lists[field_list];
}
