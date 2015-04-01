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

#include <p4utils/tommylist.h>

#include "value_set.h"

typedef tommy_list value_set_t;

//:: for name, vs_info in value_sets.items():
typedef struct value_set_${name}_entry_s {
//::   byte_width = vs_info["byte_width"]
  uint8_t value[${byte_width}];
  uint8_t mask[${byte_width}];
  tommy_node node;
} value_set_${name}_entry_t;

//:: #endfor

//:: for name, vs_info in value_sets.items():
value_set_t value_set_${name};
//:: #endfor

//:: for name, vs_info in value_sets.items():
//::   byte_width = vs_info["byte_width"]
int value_set_${name}_contains(uint8_t *key) {
  tommy_node* node = tommy_list_head(&value_set_${name});
  while (node) {
    value_set_${name}_entry_t * data = node->data;
    int result = cmp_values(key, data->value, ${byte_width}, data->mask);
    if(result)
      return 1;
    node = node->next;
  }
  return 0;
}

//:: #endfor

//:: for name, vs_info in value_sets.items():
//::   byte_width = vs_info["byte_width"]
void *value_set_${name}_add(uint8_t *value, uint8_t *mask) {
  value_set_${name}_entry_t *entry = malloc(sizeof(value_set_${name}_entry_t));
  memcpy(entry->value, value, sizeof(entry->value));
  memcpy(entry->mask, mask, sizeof(entry->mask));
  tommy_list_insert_head(&value_set_${name}, &entry->node, entry);
  return &entry->node;
}

//:: #endfor

//:: for name, vs_info in value_sets.items():
int value_set_${name}_delete(void *value) {
  void *entry = tommy_list_remove_existing(&value_set_${name}, value);
  if(!entry)
    return 1;
  free(entry);
  return 0;
}

//:: #endfor

//:: for name, vs_info in value_sets.items():
//::   byte_width = vs_info["byte_width"]
void *value_set_${name}_lookup(uint8_t *value, uint8_t *mask) {
  tommy_node* node = tommy_list_head(&value_set_${name});
  while (node) {
    value_set_${name}_entry_t * data = node->data;
    if((!memcmp(data->value, value, sizeof(data->value))) &&
       (!memcmp(data->mask, mask, sizeof(data->mask))))
      return &data->node;
    node = node->next;
  }
  return NULL;
}

//:: #endfor
