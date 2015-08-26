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

#ifndef _RMT_STATEFUL_H
#define _RMT_STATEFUL_H

#include <stdint.h>
#include "enums.h"
#include "phv.h"

typedef enum meter_color_e {
  METER_EXCEED_ACTION_COLOR_GREEN = 0,
  METER_EXCEED_ACTION_COLOR_YELLOW,
  METER_EXCEED_ACTION_COLOR_RED,
  METER_EXCEED_ACTION_COLOR_END_,
} meter_color_t;

typedef struct meter_s meter_t;
typedef struct counter_s counter_t;
typedef struct reg_s reg_t;

/* TODO: make something more like field lists ? */

//:: for c_name, c_info in counter_info.items():
//::   type_ = c_info["type_"]
//::   if type_ == "packets_and_bytes": continue
extern counter_t counter_${c_name};
//:: #endfor

//:: for m_name, m_info in meter_info.items():
extern meter_t meter_${m_name};
//:: #endfor

//:: for r_name, r_info in register_info.items():
extern reg_t reg_${r_name};
//:: #endfor

void stateful_init(void);

void stateful_init_counters(counter_t *counter, size_t num_instances, char *name);
uint64_t stateful_read_counter(counter_t *counter, int index);
void stateful_write_counter(counter_t *counter, int index, uint64_t value);
void stateful_increase_counter(counter_t *counter, int index, uint64_t value);
void stateful_reset_counter(counter_t *counter, int index);
void stateful_clean_all(void);

void stateful_init_meters(meter_t *meter, size_t num_instances, char *name);
void stateful_reset_meter(meter_t *meter, int index);
int stateful_meter_set_queue(meter_t *meter, int index,
			     double info_rate, uint32_t burst_size,
			     meter_color_t color);
meter_color_t stateful_execute_meter(meter_t *meter, int index, uint32_t input);

void stateful_init_registers(reg_t *reg, size_t num_instances, size_t byte_width);

//:: for r_name, r_info in register_info.items():
//::   byte_width = r_info["byte_width"]
void stateful_write_register_${r_name}(int index, uint8_t *src, int src_len);
//:: #endfor

//:: for r_name, r_info in register_info.items():
//::   byte_width = r_info["byte_width"]
void stateful_read_register_${r_name}(phv_data_t *phv, int index,
				      rmt_field_instance_t dst, int dst_len,
				      uint8_t *mask_ptr, int mask_len);
//:: #endfor

#endif
