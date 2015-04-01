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

#include "field_list.h"

//:: for name, field_list in field_lists.items():
field_list_entry_t ${name}_entries[] = {
//::   for type_, value in field_list:
//::     if type_ == "field_ref":
  {FIELD_LIST_ENTRY_FIELD, {(rmt_header_instance_t)RMT_FIELD_INSTANCE_${value}}, 0}, // cannot be virtual
//::     elif type_ == "header_ref":
  {FIELD_LIST_ENTRY_HEADER, {RMT_HEADER_INSTANCE_${value}}, 0}, // cannot be virtual
//::     #endif
//::   #endfor
};

//:: #endfor

field_list_t _rmt_field_lists[RMT_FIELD_LIST_COUNT + 1] = {
  {0, NULL},
//:: for name, field_list in field_lists.items():
//::   size = len(field_list)
  {${size}, ${name}_entries},
//:: #endfor
  {0, NULL}
};

