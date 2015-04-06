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

#ifndef _P4_TABLES_IF_H
#define _P4_TABLES_IF_H

#define TABLES_DEFAULT_INIT_SIZE 65536 // entries

//:: rocker_p4_prefix = "rocker_" + p4_prefix + "_"

//:: for table_name, t_info in table_info.items():
//::   key_width = t_info["key_byte_width"]
//::   action_data_width = t_info["action_data_byte_width"]
//::   match_type = t_info["match_type"]
typedef struct ${rocker_p4_prefix}${table_name}_entry_s {
  unsigned char key[${key_width}];
//::   if match_type == "ternary":
  unsigned char mask[${key_width}];
  int priority;
#ifdef P4RMT_USE_TCAM_CACHE
  bool do_not_cache; /* use for deletion, to reduce the amount of time we keep
			the write lock for the table */
#endif
//::   elif match_type == "lpm":
  int prefix_width;
//::   #endif
  int action_id;
  unsigned char action_data[${action_data_width}];

//:: if t_info['support_timeout'] is True:
  // Variables that are accessed simultaneously from multiple threads are set
  // defined volatile.
  volatile unsigned int is_hit;
  volatile unsigned int is_hit_after_last_sweep;
  struct timeval last_hit_time;
  volatile unsigned int is_reported_hit;
  volatile struct timeval ttl;
//:: #endif
} ${rocker_p4_prefix}${table_name}_entry_t;


extern void ${p4_prefix}_tables_add_entry_${table_name}(void *entry);
extern void ${p4_prefix}_tables_set_default_${table_name}(int action_id, unsigned char *action_data);
extern void ${p4_prefix}_tables_print_entries_${table_name}(void);

//:: #endfor

#endif
