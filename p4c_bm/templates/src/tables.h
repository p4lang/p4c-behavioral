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

#ifndef _RMT_TABLES_H
#define _RMT_TABLES_H

#include "enums.h"
#include "actions.h"
#include "phv.h"
#include "tcam_cache.h"
#include "stateful.h"

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#include <p4utils/tommyhashlin.h>
#include <p4utils/cheap_trie.h>
#include <p4utils/cheap_tcam.h>

#include <p4_sim/pd.h>

#include <Judy.h>

#define TABLES_DEFAULT_INIT_SIZE 65536 // entries

#define TERNARY_CACHE_THREAD_SLEEP_SECS 1
#define TERNARY_CACHE_VALIDITY_SECS 2

//:: def get_node_type(match_type):
//::   if match_type == "lpm":
//::     node_type = "void *"
//::   elif match_type == "exact":
//::     node_type = "tommy_hashlin_node"
//::   elif match_type == "ternary":
//::     node_type = "cheap_tcam_node"
//::   else:
//::     node_type = "void *"
//::   #endif
//::   return node_type
//:: #enddef

typedef void (*ApplyTableFn)(phv_data_t *phv);

//:: for table_name, t_info in table_info.items():
//::   key_width = t_info["key_byte_width"]
//::   action_data_width = t_info["action_data_byte_width"]
//::   match_type = t_info["match_type"]
//::   node_type = get_node_type(match_type)
typedef struct ${table_name}_entry_s {
  uint8_t key[${key_width}];
//::   if match_type == "ternary":
  uint8_t mask[${key_width}];
  int priority;
#ifdef P4RMT_USE_TCAM_CACHE
  bool do_not_cache; /* use for deletion, to reduce the amount of time we keep
			the write lock for the table */
#endif
//::   elif match_type == "lpm":
  int prefix_width;
//::   #endif
  int action_id;
  uint8_t action_data[${action_data_width}];
  ${node_type} node;

//:: if t_info['support_timeout'] is True:
  // Variables that are accessed simultaneously from multiple threads are set
  // defined volatile.
  volatile sig_atomic_t is_hit;
  volatile sig_atomic_t is_hit_after_last_sweep;
  struct timeval last_hit_time;
  volatile sig_atomic_t is_reported_hit;
  volatile struct timeval ttl;
//:: #endif
} ${table_name}_entry_t;

//:: #endfor

typedef struct table_s {
  rmt_table_type_t type;
  void *entries_array;
  union {
    tommy_hashlin *hash_table;
    cheap_trie_t *trie;
    cheap_tcam_t *tcam;
    void *other;
  } entries_nodes; /* hash map, trie ... */
  int max_size;
  Pvoid_t jarray_used; // Judy array of used entries
  Pvoid_t jarray_dirty;
  int default_action_id;
  uint8_t *default_action_data;
  ActionFn *actions; // list of actions, indexed by action id
  ApplyTableFn *action_next;
  ApplyTableFn hit_next;
  ApplyTableFn miss_next;
  uint64_t counter_miss_bytes;
  uint64_t counter_miss_packets;
  uint64_t counter_hit_bytes;
  uint64_t counter_hit_packets;
  pthread_mutex_t counters_lock;
  counter_t *bytes_counters;
  counter_t *packets_counters;
  meter_t *meters;
  pthread_rwlock_t lock;
#ifdef P4RMT_USE_TCAM_CACHE
  /* for use by ternary table only */
  tcam_cache_t **tcam_cache;
#endif

  p4_pd_notify_timeout_cb timeout_cb_fn;
  void *timeout_client_data;
} table_t;

/* Accessor functions for (static) table properties. */

//:: table_gen_objs = [				\
//::     ("min_size", "int"),			\
//::     ("max_size", "int"),			\
//::     ("type", "rmt_table_type_t"),		\
//:: ]

//:: for obj_name, ret_type in table_gen_objs:

extern ${ret_type} _rmt_table_${obj_name}[RMT_TABLE_COUNT + 1];

static inline ${ret_type}
rmt_table_${obj_name}(rmt_table_t table)
{
  return _rmt_table_${obj_name}[table];
}
//:: #endfor


//:: for table, t_info in table_info.items():
void tables_apply_${table}(phv_data_t *phv);
//:: #endfor

//:: for ctable, ct_info in conditional_table_info.items():
void tables_apply_${ctable}(phv_data_t *phv);
//:: #endfor

//:: for table, t_info in table_info.items():
int tables_set_default_${table}(int action_id, uint8_t *action_data);
//:: #endfor

//:: for table, t_info in table_info.items():
int tables_add_entry_${table}(void *entry);
//:: #endfor

//:: for table, t_info in table_info.items():
int tables_delete_entry_${table}(int entry_index);
//:: #endfor

//:: for table, t_info in table_info.items():
int tables_modify_entry_${table}(int entry_index, int action_id, uint8_t *action_data);
//:: #endfor

//:: counter_types = ["bytes", "packets"]
//:: for type_ in counter_types:
//::   for table, t_info in table_info.items():
uint64_t tables_read_${type_}_counter_hit_${table}(void);
void tables_reset_${type_}_counter_hit_${table}(void);
uint64_t tables_read_${type_}_counter_miss_${table}(void);
void tables_reset_${type_}_counter_miss_${table}(void);
//::   #endfor
//:: #endfor

/* for testing, wrapper functions around the _lookup_* */
//:: for table, t_info in table_info.items():
void *tables_lookup_${table}(uint8_t *key);
//:: #endfor

/* To print the entries in a table */
//:: for table, t_info in table_info.items():
void tables_print_entries_${table}(void);
//:: #endfor

//:: for table, _ in table_info.items():
int
tables_${table}_get_first_entry_handle(void);
void
tables_${table}_get_next_entry_handles(int entry_handle, int n, int *next_entry_handles);
void
tables_${table}_get_entry_string(p4_pd_entry_hdl_t entry_hdl, char *entry_desc, int max_entry_length);
//:: #endfor

int tables_clean_all(void);

void tables_init(void);

//:: for table_name in [t for t in table_info.keys() if table_info[t]['support_timeout']]:
void
tables_${table_name}_get_hit_state(const int entry_index, bool *const is_hit);

p4_pd_status_t
tables_${table_name}_set_entry_ttl(const int entry_hdl, const uint32_t ttl);

void
tables_${table_name}_enable_entry_timeout(p4_pd_notify_timeout_cb cb_fn,
                                          const uint32_t max_ttl,
                                          void *client_data);
//:: #endfor

void tables_print(rmt_table_t table);

int tables_get_action_id(rmt_table_t table, ActionFn act_fn);

#endif
