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

#ifndef _RMT_VALUE_SET_H
#define _RMT_VALUE_SET_H

#include <string.h>

#include "fields.h"

//:: for name, vs_info in value_sets.items():
int value_set_${name}_contains(uint8_t *key);
//:: #endfor

//:: for name, vs_info in value_sets.items():
void *value_set_${name}_add(uint8_t *value, uint8_t *mask);
//:: #endfor

//:: for name, vs_info in value_sets.items():
int value_set_${name}_delete(void *value);
//:: #endfor

//:: for name, vs_info in value_sets.items():
void *value_set_${name}_lookup(uint8_t *value, uint8_t *mask);

//:: #endfor

#endif
