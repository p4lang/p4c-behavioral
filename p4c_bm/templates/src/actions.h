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

#ifndef _RMT_ACTIONS_H
#define _RMT_ACTIONS_H

#include <stdint.h>

#include "phv.h"

/* Generic type for action functions, table entries will store a function
   pointer to the appropriate such function */
typedef void (*ActionFn)(phv_data_t *phv, void *action_data);

//:: for action_name, action in action_info.items():
void action_${action_name} (phv_data_t *phv, void *action_data);
//:: #endfor

char *action_get_name(ActionFn fn);
void action_dump_action_data(ActionFn fn, uint8_t *data, char *action_desc, int max_length);

#endif
