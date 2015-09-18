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

#include "tables.h"
#include "action_profiles.h"
#include "enums.h"
#include "phv.h"
#include "fields.h"
#include "rmt_internal.h"
#include "actions.h"
#include "stateful.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <limits.h>

#include <p4utils/lookup3.h>

pthread_t ternary_cache_thread;
pthread_t entry_aging_thread;

//:: table_gen_objs = [				\
//::     ("min_size", "int"),			\
//::     ("max_size", "int"),			\
//::     ("type", "rmt_table_type_t"),		\
//:: ]
//::
//:: def gen_table_map_ref(obj_name, t_info, comma):
//::     width_str = "%-25s"
//::     if obj_name in ["min_size", "max_size"]:
//::         width_str = "%-4s"
//::         obj_str = "-1" + comma  # Default if t_info is None
//::         if t_info:
//::           size = -1 if not t_info[obj_name] else t_info[obj_name]
//::           obj_str = str(size) + comma
//::         #endif
//::     elif obj_name == "type":
//::         obj_str = "RMT_TABLE_TYPE_COUNT" + comma
//::         if t_info:
//::             match_type = t_info["match_type"]
//::             obj_str = "RMT_TABLE_TYPE_" + match_type.upper() + comma
//::         #endif
//::     #endif
//::     return width_str % obj_str
//:: #enddef
//::
//:: def increase_counter(counter_str, value, c_info):
//::   if c_info["saturating"]:
//::     return "if((uint64_t) (%s + %s) < %s) %s += %s" % (counter_str, value,
//::                                                        counter_str,
//::                                                        counter_str, value)
//::   else:
//::     return "%s += %s" % (counter_str, value)
//::   #endif
//:: #enddef

//:: for obj_name, ret_type in table_gen_objs:

/****************************************************************/

/*
 * Map from table to ${obj_name}
 */

${ret_type} _rmt_table_${obj_name}[RMT_TABLE_COUNT + 1] = {
  ${gen_table_map_ref(obj_name, None, ",")} /* RMT_TABLE_NONE */
  //::     for table, t_info in table_info.items():
  ${gen_table_map_ref(obj_name, t_info, ",")} /* RMT_TABLE_${table} */
  //::     #endfor
  ${gen_table_map_ref(obj_name, None, "")} /* RMT_TABLE_COUNT */
};
//:: #endfor

/*
 * Map from table back to name
 */

const char *rmt_table_string[RMT_TABLE_COUNT + 1] = {
  "INVALID (0)", /* NONE */
//:: for table, t_info in table_info.items():
  "${table}",
//:: #endfor
  "INVALID (max)"
};


table_t tables_array[RMT_TABLE_COUNT + 1];


/* Compare functions for search */

//:: for table, t_info in table_info.items():
/*static inline */ int entry_cmp_${table}(const void *key, const void *entry) {
//::   key_width = t_info["key_byte_width"]
  return memcmp(key, ((${table}_entry_t *)entry)->key, ${key_width});
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   if match_type == "ternary":
/*static inline */ int entry_cmp_${table}_cache(const void *key, const void *entry) {
//::     key_width = t_info["key_byte_width"]
  ${table}_entry_t *table_entry = (${table}_entry_t *) entry;
  int i;
  (void)table_entry; /* Compiler warning */
  for(i = 0; i < ${key_width}; i++) {
    if(table_entry->key[i] != (((uint8_t *) key)[i] & table_entry->mask[i]))
      return 1;
  }
  return 0;
}

//:: #endif
//:: #endfor

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   if match_type == "ternary":
/*static inline*/ int entry_get_priority_${table}(const void *entry) {
//::     key_width = t_info["key_byte_width"]
  return ((${table}_entry_t *)entry)->priority;
}

//:: #endif
//:: #endfor

/* the action list for the table, indexed by action id (table specific) */
//:: for table, t_info in table_info.items():
//::   actions = t_info["actions"]
//::   num_actions = len(actions)
ActionFn ${table}_actions[${num_actions}] = {
//::   for action in actions:
  action_${action},
//::   #endfor
};

//:: #endfor

static ActionFn *tables_actions_reverse[] = {
  NULL,
//:: for table, t_info in table_info.items():
  ${table}_actions,
//:: #endfor
  NULL
};

int tables_get_action_id(rmt_table_t table, ActionFn act_fn){
  ActionFn *actions = tables_actions_reverse[table];
  int action_id;
  for(action_id = 0; ; action_id++) {
    if(actions[action_id] == act_fn) return action_id;
  }
  return -1;
}

/* the next tables for a given table, indexed by action id, if supported by that
   table */
//:: for table, t_info in table_info.items():
//::   if t_info["with_hit_miss_spec"]: continue
//::   actions = t_info["actions"]
//::   num_actions = len(actions)
ApplyTableFn ${table}_action_next[${num_actions}] = {
//::   for action in actions:
//::     next_table = t_info["next_tables"][action]
//::     if next_table:
  tables_apply_${next_table},
//::     else:
  NULL,
//::     #endif
//::   #endfor
};

//:: #endfor


//:: for table, t_info in table_info.items():
static void ${table}_init(void){
  table_t *table = &tables_array[RMT_TABLE_${table}];
//::   t_size = t_info["max_size"]
//::   if not t_size or t_size <= 1: t_size = "TABLES_DEFAULT_INIT_SIZE"
  table->entries_array = malloc(${t_size} *
				sizeof(${table}_entry_t));

//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
  table->type = RMT_TABLE_TYPE_${match_type.upper()};

//::   if match_type == "exact":
  table->entries_nodes.hash_table = malloc(sizeof(tommy_hashlin));
  tommy_hashlin_init(table->entries_nodes.hash_table);
//::   elif match_type == "lpm":
  table->entries_nodes.trie = cheap_trie_create(${key_width});
//::   elif match_type == "ternary":
#ifdef P4RMT_USE_CHEAP_TCAM
  table->entries_nodes.tcam = cheap_tcam_create(${key_width},
						entry_get_priority_${table},
						entry_cmp_${table});
#else
  table->entries_nodes.other = malloc(sizeof(int));
#endif
#ifdef P4RMT_USE_TCAM_CACHE
  table->tcam_cache = malloc(NB_THREADS * sizeof(void *));
  int i;
  for(i = 0; i < NB_THREADS; i++) {
    table->tcam_cache[i] = tcam_cache_create(256, ${key_width},
					     TERNARY_CACHE_VALIDITY_SECS);
  }
#endif
//::   else:
  table->entries_nodes.other = malloc(sizeof(int));
//::   #endif

  table->max_size = ${t_size};
  table->jarray_used = (Pvoid_t) NULL;
  table->jarray_dirty = (Pvoid_t) NULL;

//::   action_data_width = t_info["action_data_byte_width"]
  table->default_action_data = malloc(${action_data_width});
  table->default_action_id = -1;

  table->actions = ${table}_actions;

//::   hit_next = None
//::   miss_next = None
//::   if t_info["with_hit_miss_spec"]:
//::     hit_next = t_info["next_tables"]["hit"]
//::     miss_next = t_info["next_tables"]["miss"]
//::   #endif
//::   if hit_next:
  table->hit_next = tables_apply_${hit_next};
//::   else:
  table->hit_next = NULL;
//::   #endif
//::   if miss_next:
  table->miss_next = tables_apply_${miss_next};
//::   else:
  table->miss_next = NULL;
//::   #endif

//::   if t_info["with_hit_miss_spec"]:
  table->action_next = NULL;
//::   else:
  table->action_next = ${table}_action_next;
//::   #endif

  table->counter_miss_bytes = 0;
  table->counter_hit_bytes = 0;
  table->counter_miss_packets = 0;
  table->counter_hit_packets = 0;
  pthread_mutex_init(&table->counters_lock, NULL);

//::   if t_info["bytes_counter"]:
//::     c_name = t_info["bytes_counter"]
  counter_t *counter_bytes = &counter_${t_info["bytes_counter"]};
  stateful_init_counters(counter_bytes, ${t_size}, "${c_name}");
  table->bytes_counters = counter_bytes;
//::   #endif
//::   if t_info["packets_counter"]:
//::     c_name = t_info["packets_counter"]
  counter_t *counter_packets = &counter_${t_info["packets_counter"]};
  stateful_init_counters(counter_packets, ${t_size}, "${c_name}");
  table->packets_counters = counter_packets;
//::   #endif

//::   if t_info["meter"]:
  meter_t *meter = &meter_${t_info["meter"]};
  stateful_init_meters(meter, ${t_size}, "${t_info['meter']}");
  table->meters = meter;
//::   #endif

//::   for r_name in t_info["registers"]:
//::     r_info = register_info[r_name]
  stateful_init_registers(&reg_${r_name}, ${t_size}, ${r_info["byte_width"]});
//::   #endfor

  table->timeout_cb_fn = NULL;

  pthread_rwlock_init(&table->lock, NULL);
}

//:: #endfor

#ifdef P4RMT_USE_TCAM_CACHE

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   if match_type != "ternary": continue
static void ${table}_cache_cleanup(void) {
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "cleaning up cache for ternary table ${table}\n");

  table_t *table = &tables_array[RMT_TABLE_${table}];

  tcam_cache_t *tcam_cache;
  int i;
  int removed = 0;
  for (i = 0; i < NB_THREADS; i++) {
    tcam_cache = table->tcam_cache[i];
    removed += tcam_cache_purge(tcam_cache);
  }
      
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "cleaned up cache for ternary table ${table}: %d entries expired\n",
	  removed);
}

//:: #endfor

void *ternary_cache_cleanup(void *arg) {
  rmt_table_t table_index;
  while(1) {
//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   if match_type != "ternary": continue
    ${table}_cache_cleanup();
//:: #endfor

    sleep(TERNARY_CACHE_THREAD_SLEEP_SECS);
  }
}

#endif // P4RMT_USE_TCAM_CACHE

#ifdef DEBUG
/* for debugging purposes, print an entry in a nice way */

/* IMPORTANT: does not acquire the lock, which means caller must have either
   writer's lock or reader's lock */

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
static void
_tables_${table}_get_entry_string(p4_pd_entry_hdl_t entry_hdl, char *entry_desc, int max_entry_length);

static void print_entry_${table}(${table}_entry_t *entry) {
  table_t *table = &tables_array[RMT_TABLE_${table}];

  ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;
  int index = entry - entries;
  RMT_PRINT("\n**********\n");
  RMT_PRINT("entry at index %d:\n", index);

  char buffer[4096];
  _tables_${table}_get_entry_string(index, buffer, 4096);
  RMT_PRINT("%s\n", buffer);

//::   if t_info["bytes_counter"] or t_info["packets_counter"]:
//::     if t_info["bytes_counter"]:
  RMT_PRINT("\tbytes counter:\n\t\t");
  RMT_PRINT("%u\n", stateful_read_counter(table->bytes_counters, index));
//::     #endif
//::     if t_info["packets_counter"]:
  RMT_PRINT("\tpackets counter:\n\t\t");
  RMT_PRINT("%u\n", stateful_read_counter(table->packets_counters, index));
//::     #endif
//::   #endif

  /* TODO: print something for meters ? */

  RMT_PRINT("**********\n\n");
}

//:: #endfor
#endif

typedef int (*EntryDelFn)(int entry_index);
typedef int (*EntryModFn)(int entry_index, int action_id, uint8_t *action_data);

static inline int get_and_set_index(table_t *table) {
  Word_t index = 0;
  int Rc_int;
  J1FE(Rc_int, table->jarray_used, index); // Judy1FirstEmpty()
  for (;index < table->max_size;) {
    J1T(Rc_int, table->jarray_dirty, index);
    if (!Rc_int) {
      break;
    }
    J1NE(Rc_int, table->jarray_used, index);
  }
  if (index < table->max_size) {
    J1S(Rc_int, table->jarray_used, index); // Judy1Set()
    assert(Rc_int);
  }
  return (index < table->max_size ? (int)index : -1);
}

static inline int unset_index(table_t *table, Word_t index) {
  int Rc_int;
  J1U(Rc_int, table->jarray_used, index); // Judy1Unset()
  assert(Rc_int);
  return Rc_int;
}

static inline int test_index(const Pvoid_t *jarray, Word_t index) {
  int Rc_int;
  J1T(Rc_int, *jarray, index);
  return Rc_int;
}

#ifdef P4RMT_USE_TCAM_CACHE

static void invalidate_cache(table_t *table){
  int i;
  tcam_cache_t *tcam_cache;
  for (i = 0; i < NB_THREADS; i++) {
    tcam_cache = table->tcam_cache[i];
    tcam_cache_invalidate(tcam_cache);
  }
}

#endif

//:: for table, t_info in table_info.items():
//::   if not t_info["bytes_counter"] and not t_info["packets_counter"]: continue
void static reset_counters_${table}(int entry_index) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
//::   if t_info["bytes_counter"]:
  stateful_reset_counter(table->bytes_counters, entry_index);
//::   #endif
//::   if t_info["packets_counter"]:
  stateful_reset_counter(table->packets_counters, entry_index);
//::   #endif
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   if not t_info["meter"]: continue
void static reset_meter_${table}(int entry_index) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  stateful_reset_meter(table->meters, entry_index);
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
int tables_add_entry_${table}(void *entry) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "${table}: adding entry\n");
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  int empty_index = get_and_set_index(table);
  if (empty_index < 0) {
    Word_t rc_used, rc_dirty;
    J1C(rc_used, table->jarray_used, 0, -1);
    J1C(rc_dirty, table->jarray_dirty, 0, -1);
    RMT_LOG(P4_LOG_LEVEL_WARN,
            "Table full. %d used entries, %d dirty entries\n",
            (int)rc_used, (int)rc_dirty);
    pthread_rwlock_unlock(&table->lock);
    return empty_index;
  }

  RMT_LOG(P4_LOG_LEVEL_TRACE, "empty index: %d\n", empty_index);
  ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;
  entries[empty_index] = *(${table}_entry_t *) entry;

//:: if t_info['support_timeout'] is True:
  ${table}_entry_t *const new_entry = &(entries[empty_index]);
  new_entry->is_hit = (sig_atomic_t)0;
  // Default parameters for callback mode are set to the state where the entry
  // has just been hit.
  new_entry->is_hit_after_last_sweep = (sig_atomic_t)0;
  new_entry->is_reported_hit = (sig_atomic_t)1;
  gettimeofday(&(new_entry->last_hit_time), NULL);
//:: #endif

//::   if match_type == "ternary":
  /* make sure that the key has been masked, cheap_tcam requires it */
  int i;
  for(i = 0; i < ${key_width}; i++) {
    entries[empty_index].key[i] &= entries[empty_index].mask[i];
  }
//::   #endif

//::   if match_type == "exact":
  uint32_t hash = hashlittle(entries[empty_index].key, ${key_width}, 0);
  tommy_hashlin_insert(table->entries_nodes.hash_table,
		       &(entries[empty_index].node),
		       &(entries[empty_index]),
		       hash);
//::   elif match_type == "lpm":
  cheap_trie_insert(table->entries_nodes.trie,
		    entries[empty_index].key,
		    entries[empty_index].prefix_width,
		    &entries[empty_index]);
//::   elif match_type == "ternary":
#ifdef P4RMT_USE_TCAM_CACHE
  entries[empty_index].do_not_cache = 0;
#endif
#ifdef P4RMT_USE_CHEAP_TCAM
  cheap_tcam_insert(table->entries_nodes.tcam,
		    entries[empty_index].mask,
		    entries[empty_index].key,
		    &(entries[empty_index].node),
		    &(entries[empty_index]));
#endif
//::   else:
//::     pass
//::   #endif
#ifdef DEBUG
  if(P4_LOG_LEVEL_TRACE <= RMT_LOG_LEVEL)
    print_entry_${table}(&entries[empty_index]);
#endif

//::   if t_info["bytes_counter"] or t_info["packets_counter"]:
  reset_counters_${table}(empty_index);

//::   #endif
//::   if t_info["meter"]:
  reset_meter_${table}(empty_index);

//::   #endif
  pthread_rwlock_unlock(&table->lock);
//:: if match_type == "ternary":
#ifdef P4RMT_USE_TCAM_CACHE
  /* Check that we don't need the table lock here */
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "invalidating caches for ternary table ${table}\n");
  invalidate_cache(table);
#endif
//:: #endif

  return empty_index;
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
int tables_delete_entry_${table}(int entry_index) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Deleting entry %d in ${table}\n", entry_index);
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  if(!test_index(&table->jarray_used, entry_index)){
    RMT_LOG(P4_LOG_LEVEL_WARN, "Invalid entry index %d in ${table}\n",
            entry_index);
    pthread_rwlock_unlock(&table->lock);
    return 1;
  }

  ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;
  ${table}_entry_t *entry = &entries[entry_index];

  (void)entry; /* COMPILER REFERENCE */
//::   if match_type == "ternary":
#ifdef P4RMT_USE_TCAM_CACHE
  /* a "trick" to avoid having to keep the wrlock while invalidating the
     cache. The entry is marked as "not cachable", but it can still be read from
     one cache while we invalidate the others. Maybe it's lot of trouble for
     not that much gain, we'll see. */
  entry->do_not_cache = 1;
  pthread_rwlock_unlock(&table->lock);
  invalidate_cache(table);
  pthread_rwlock_wrlock(&table->lock);
#endif
//::   #endif

//::   if match_type == "exact":
  tommy_hashlin_remove_existing(table->entries_nodes.hash_table,
				&entry->node);

//::   elif match_type == "lpm":
  ${table}_entry_t *e = cheap_trie_delete(table->entries_nodes.trie,
					  entry->key,
					  entry->prefix_width);
  assert(e);
//::   elif match_type == "ternary":
#ifdef P4RMT_USE_CHEAP_TCAM
  cheap_tcam_delete(table->entries_nodes.tcam,
		    entry->mask,
		    entry->key,
		    &entry->node);
#else
  (void) entry; /* compiler reference */
#endif
//::   else:
  (void) entry; /* compiler reference */
//::   #endif

  int result = !unset_index(table, entry_index);

  pthread_rwlock_unlock(&table->lock);
  
  return result;
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
//::   action_data_width = t_info["action_data_byte_width"]
int tables_modify_entry_${table}(int entry_index,
				 int action_id, uint8_t *action_data) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "${table}: modifying entry\n");
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  if(!test_index(&table->jarray_used, entry_index)) {
    pthread_rwlock_unlock(&table->lock);
    return -1;
  }

  if(test_index(&table->jarray_dirty, entry_index)) {
    RMT_LOG(P4_LOG_LEVEL_WARN,
            "Entry %d is dirty in ${table}, aborting entry modification\n",
            entry_index);
    pthread_rwlock_unlock(&table->lock);
    return -1;
  }

  ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;
  ${table}_entry_t *entry = &entries[entry_index];
  entry->action_id = action_id;
  memcpy(entry->action_data, action_data, ${action_data_width});

  pthread_rwlock_unlock(&table->lock);

//:: if match_type == "ternary":
#ifdef P4RMT_USE_TCAM_CACHE
  /* Check that we don't need the table lock here */
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "invalidating caches for ternary table ${table}\n");
  invalidate_cache(table);
#endif
//:: #endif

  return 0;
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   action_data_width = t_info["action_data_byte_width"]
int tables_set_default_${table}(int action_id, uint8_t *action_data) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "${table}: setting default action\n");
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  table->default_action_id = action_id;
  memcpy(table->default_action_data, action_data, ${action_data_width});

  pthread_rwlock_unlock(&table->lock);

  return 0;
}

//:: #endfor

/* Build key functions */

//:: for table, t_info in table_info.items():
static inline void build_key_${table}(phv_data_t *phv, uint8_t *key) {
  /* has to be determined at runtime because of virtual instances */
//::   if t_info["has_mask"]:
  uint8_t *key_mask = key;
//::   #endif
  int byte_offset_phv;
  (void)byte_offset_phv; /* Compiler reference */
//::   key_width = t_info["key_byte_width"]
//::   for field, m_type in t_info["reordered_match_fields"]:
//::     if m_type == "valid":
  *(key++) = (uint8_t ) phv_is_valid_header(phv, PHV_GET_HEADER(phv, RMT_HEADER_INSTANCE_${field}));
//::       continue
//::     #endif
//::     f_info = field_info[field]
  byte_offset_phv = rmt_field_byte_offset_phv(PHV_GET_FIELD(phv, RMT_FIELD_INSTANCE_${field}));
//::       if f_info["data_type"] == "uint32_t":
  *(uint32_t *) key = phv_buf_field_uint32_get(phv,byte_offset_phv);
  key += sizeof(uint32_t);
//::       elif f_info["data_type"] == "byte_buf_t":
//::         byte_width = f_info["byte_width_phv"]
  memcpy(key, phv_buf_field_byte_buf_get(phv, byte_offset_phv), ${byte_width});
  key += ${byte_width};
//::     #endif
//::   #endfor
//::   if t_info["has_mask"]:
  uint8_t big_mask[${key_width}] = {
//::     for c in t_info["big_mask"]:
    ${c},
//::     #endfor
  };
//::     if key_width <= 4:
  *(uint32_t *) key_mask = (*(uint32_t *) key_mask) & (*(uint32_t) big_mask);
//::     else:
  int i;
  for(i = 0; i < ${key_width}; i++) {
    key_mask[i] &= big_mask[i];
  }
//::     #endif
//::   #endif
}

//:: #endfor

//:: for table, t_info in table_info.items():
static inline void *_lookup_${table}(table_t *table, uint8_t *key, int id) {
//::   key_width = t_info["key_byte_width"]
//::   match_type = t_info["match_type"]
  ${table}_entry_t *entry = NULL;
//::   if match_type == "exact":
  uint32_t hash = hashlittle(key, ${key_width}, 0);
  entry = tommy_hashlin_search(table->entries_nodes.hash_table,
			       entry_cmp_${table},
			       key,
			       hash);
//::   elif match_type == "lpm":
  entry = cheap_trie_get(table->entries_nodes.trie, key);
//::   elif match_type == "ternary":
#ifdef P4RMT_USE_TCAM_CACHE
  int cache_status;
  tcam_cache_t *tcam_cache = table->tcam_cache[id];
  cache_status = tcam_cache_lookup(tcam_cache, key, (void **) &entry);
  if(entry){
    RMT_LOG(P4_LOG_LEVEL_TRACE,
	    "ternary table ${table} cache hit\n");
    return entry;
  }
  assert(cache_status);
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "ternary table ${table} cache miss\n");
#endif
#ifdef P4RMT_USE_CHEAP_TCAM
  entry = cheap_tcam_search(table->entries_nodes.tcam, key);
#else
  ${table}_entry_t *entries = table->entries_array;
  int min_priority = INT_MAX;
  Word_t i = 0;
  int Rc_int;
  J1F(Rc_int, table->jarray_used, i); // Judy1First()
  for(; Rc_int; ) {
    if(entries[i].priority < min_priority) {
      int result = cmp_values(key, entries[i].key, ${key_width}, entries[i].mask);
      if(result){
        entry = &entries[i];
        min_priority = entry->priority;
      }
    }

    J1N(Rc_int, table->jarray_used, i)
  }
#endif
#ifdef P4RMT_USE_TCAM_CACHE
  if(cache_status == 1 && !entry->do_not_cache) {
    RMT_LOG(P4_LOG_LEVEL_TRACE,
	    "ternary table ${table} adding to cache\n");
    tcam_cache_insert(tcam_cache, key, entry);
  }
#endif

//::   else:
//::     pass
//::   #endif
  return entry;
}

//:: #endfor

/* for testing, wrapper functions around the _lookup_* */
//:: for table, t_info in table_info.items():
void *tables_lookup_${table}(uint8_t *key) {
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_rdlock(&table->lock);

  ${table}_entry_t *entry = _lookup_${table}(table, key, 0);

  pthread_rwlock_unlock(&table->lock);

  return entry;
}

//:: #endfor

//:: for table, t_info in table_info.items():
/* entry_index == -1 means table miss */
static void update_counters_${table}(phv_data_t *phv, int entry_index) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  uint64_t bytes = (uint64_t) fields_get_packet_length(phv);

  pthread_mutex_lock(&table->counters_lock);
  if (entry_index  >= 0) {
    table->counter_hit_packets++;
    table->counter_hit_bytes += bytes;
  }
  else {
    table->counter_miss_packets++;
    table->counter_miss_bytes += bytes;
  }
  pthread_mutex_unlock(&table->counters_lock);

//::   if t_info["bytes_counter"] or t_info["bytes_counter"]:
  if (entry_index  >= 0) {
//::     if t_info["packets_counter"]:
    stateful_increase_counter(table->packets_counters, entry_index, 1);
//::     #endif
//::     if t_info["bytes_counter"]:
    stateful_increase_counter(table->bytes_counters, entry_index, bytes);
//::     #endif
  }
//::   #endif
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::     if not t_info["meter"]: continue
static void update_meter_${table}(phv_data_t *phv, int entry_index) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
//::     m_info = meter_info[t_info["meter"]]
//::     if m_info["type_"] == "packets":
  uint32_t meter_input = 1;
//::     else:
  uint32_t meter_input = (uint32_t) fields_get_packet_length(phv);
//::     #endif
  uint32_t color;
  color = (uint32_t) stateful_execute_meter(table->meters, entry_index, meter_input);
//::     m_dst = m_info["result"]
//::     f_info = field_info[m_dst]
//::     byte_offset_phv = f_info["byte_offset_phv"]
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, ntohl(color));
}

//:: #endfor

//:: for table, t_info in table_info.items():
void tables_apply_${table}(phv_data_t *phv) {
  RMT_LOG(P4_LOG_LEVEL_TRACE,
	  "Applying table %s\n", rmt_table_string[RMT_TABLE_${table}]);

  table_t *table = &tables_array[RMT_TABLE_${table}];
//::   key_width = t_info["key_byte_width"]
  uint8_t key[${key_width}];
  build_key_${table}(phv, key);

  /* TODO: copy action fn ptr and data and release lock instead of keeping it
     when executing action ? */
  pthread_rwlock_rdlock(&table->lock);

  ${table}_entry_t *entry = _lookup_${table}(table, key, phv->id);
  int action_id;

  (void)entry; /* Compiler warning */

  if(entry) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "table hit\n");
#ifdef DEBUG
    if(P4_LOG_LEVEL_TRACE <= RMT_LOG_LEVEL)
      print_entry_${table}(entry);
#endif
    action_id = entry->action_id;
//:: if t_info['support_timeout'] is True:
    entry->is_hit = (sig_atomic_t)1;
    entry->is_hit_after_last_sweep = (sig_atomic_t)1;
//:: #endif
  }
  else { /* default action */
    RMT_LOG(P4_LOG_LEVEL_TRACE, "table miss, applying default action\n");
    /* assert(table->default_action); */
    if(table->default_action_id >= 0){
      action_id = table->default_action_id;
    }
    else{
      RMT_LOG(P4_LOG_LEVEL_WARN, "no default action, doing nothing\n");
      action_id = -1;
    }
  }

  int index = -1;
  if(entry) {
    ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;
    index = entry - entries;
  }

  if(action_id >= 0) {
    phv->last_entry_hit = index;
    char *action_data;
    if (!entry)
      action_data = (char *) table->default_action_data;
    else
      action_data = (char *) entry->action_data;
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None:
    table->actions[action_id](phv, action_data);
//::   else:
    uint8_t *indirect_data = (uint8_t *) action_profiles_get_data_${act_prof}(phv, action_data); 
    table->actions[action_id](phv, indirect_data);
//::   #endif
  }
  pthread_rwlock_unlock(&table->lock);

  if(entry) {
    update_counters_${table}(phv, index);
//::   if t_info["meter"]:
    update_meter_${table}(phv, index);
//::   #endif
  }
  else {
    update_counters_${table}(phv, -1);
  }

  ApplyTableFn action_next = NULL;
//::   if t_info["with_hit_miss_spec"]:
  if(entry) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "executing hit table\n");
    action_next = table->hit_next;
  }
  else {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "executing miss table\n");
    action_next = table->miss_next;
  }
//::   else:
  if(action_id >= 0) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "executing next table for action\n");
    action_next = table->action_next[action_id];
  }
//::   #endif
  if(action_next) { /* action_next NULL means last action in the pipeline */
    action_next(phv);
  }
}

//:: #endfor


/* counter functions */

//:: counter_types = ["bytes", "packets"]
//:: for type_ in counter_types:
//::   for table, t_info in table_info.items():
uint64_t tables_read_${type_}_counter_hit_${table}(void) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  pthread_mutex_lock(&table->counters_lock);  
  uint64_t counter = table->counter_hit_${type_};
  pthread_mutex_unlock(&table->counters_lock);
  return counter;
}

void tables_reset_${type_}_counter_hit_${table}(void) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  pthread_mutex_lock(&table->counters_lock);  
  table->counter_hit_${type_} = 0;
  pthread_mutex_unlock(&table->counters_lock);
}

uint64_t tables_read_${type_}_counter_miss_${table}(void) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  pthread_mutex_lock(&table->counters_lock);  
  uint64_t counter = table->counter_miss_${type_};
  pthread_mutex_unlock(&table->counters_lock);
  return counter;
}

void tables_reset_${type_}_counter_miss_${table}(void) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  pthread_mutex_lock(&table->counters_lock);  
  table->counter_miss_${type_} = 0;
  pthread_mutex_unlock(&table->counters_lock);
}

//::   #endfor
//:: #endfor

//:: for table, t_info in table_info.items():
static void ${table}_clean(void){
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  /* Cleaning up Judy index tracker */
  Word_t ret;
  J1FA(ret, table->jarray_used);
  table->jarray_used = (Pvoid_t) NULL;    
  J1FA(ret, table->jarray_dirty);
  table->jarray_dirty = (Pvoid_t) NULL;    

  /* Cleaning up overlay data structure */
//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
//::   if match_type == "exact":
  tommy_hashlin_done(table->entries_nodes.hash_table);
  tommy_hashlin_init(table->entries_nodes.hash_table);
//::   elif match_type == "lpm":
  cheap_trie_destroy(table->entries_nodes.trie);
  table->entries_nodes.trie = cheap_trie_create(${key_width});
//::   elif match_type == "ternary":
#ifdef P4RMT_USE_CHEAP_TCAM
  cheap_tcam_destroy(table->entries_nodes.tcam);
  table->entries_nodes.tcam = cheap_tcam_create(${key_width},
						entry_get_priority_${table},
						entry_cmp_${table});
#endif
#ifdef P4RMT_USE_TCAM_CACHE
  invalidate_cache(table);
#endif
//::   #endif

//::   action_data_width = t_info["action_data_byte_width"]
  table->default_action_id = -1;

//::   if t_info["bytes_counter"]:
  table->counter_miss_bytes = 0;
  table->counter_hit_bytes = 0;
//::   #endif
//::   if t_info["packets_counter"]:
  table->counter_miss_packets = 0;
  table->counter_hit_packets = 0;
//::   #endif

  pthread_rwlock_unlock(&table->lock);
}

//:: #endfor

int tables_clean_all(void) {
  RMT_LOG(P4_LOG_LEVEL_INFO,
	  "Cleaning all state (resetting all tables)\n");
  action_profiles_clean_all();
//:: for table, t_info in table_info.items():
  ${table}_clean();
//:: #endfor
  return 0;
}


/* To print the entries in a table */
//:: for table, t_info in table_info.items():
void tables_print_entries_${table}(void){
  RMT_PRINT("PRINTING ENTRIES FOR TABLE %s\n\n",
	    rmt_table_string[RMT_TABLE_${table}]);
  table_t *table = &tables_array[RMT_TABLE_${table}];
  ${table}_entry_t *entries = (${table}_entry_t *) table->entries_array;

  pthread_rwlock_wrlock(&table->lock);

  Word_t index = 0;
  int Rc_int;
  J1F(Rc_int, table->jarray_used, index); // Judy1First()
  for(; Rc_int; ) {
#ifdef DEBUG
    print_entry_${table}(&entries[index]);
#endif
    J1N(Rc_int, table->jarray_used, index);
  }

  pthread_rwlock_unlock(&table->lock);
}

//:: #endfor
//:: for table, t_info in table_info.items():
int
tables_${table}_get_first_entry_handle(void) {
  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  Word_t index = 0;
  int Rc_int;
  J1F(Rc_int, table->jarray_used, index); // Judy1First()
  if (!Rc_int) {
    index = -1;
  }

  pthread_rwlock_unlock(&table->lock);

  return index;
}

void
tables_${table}_get_next_entry_handles(int entry_handle, int n, int *next_entry_handles) {
  int i;
  for (i = 0; i < n; ++i) {
    next_entry_handles[i] = -1;
  }

  table_t *table = &tables_array[RMT_TABLE_${table}];

  pthread_rwlock_wrlock(&table->lock);

  Word_t index = (Word_t)entry_handle;
  int Rc_int = 1;
  for (i = 0; Rc_int && (i < n); ++i) {
    J1N(Rc_int, table->jarray_used, index);
    if (Rc_int) {
      next_entry_handles[i] = (int)index;
    }
  }

  pthread_rwlock_unlock(&table->lock);
}

//::   match_type = t_info["match_type"]
static void
_tables_${table}_get_entry_string(p4_pd_entry_hdl_t entry_hdl, char *entry_desc, int max_entry_length) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  ${table}_entry_t *entry = ((${table}_entry_t *) table->entries_array)+ entry_hdl;

  int bytes_output = 0;
  bytes_output += sprintf(entry_desc + bytes_output, "\tkey:\n\t\t");
  assert(bytes_output < max_entry_length);
  uint8_t *key = entry->key;
  (void)key; /* COMPILER REFERENCE */
//::   for field, m_type in t_info["reordered_match_fields"]:
//::     if m_type == "valid":
//::       byte_width = 1
  bytes_output += sprintf(entry_desc + bytes_output, "${field}_valid: %d", (int) key[0]);
  assert(bytes_output < max_entry_length);
//::     else:
//::       f_info = field_info[field]
//::       byte_width = f_info["byte_width_phv"]
//::       format_str = "%02x" * byte_width
//::       args_str = ", ".join(["key[%d]" % i for i in xrange(byte_width)])
  bytes_output += sprintf(entry_desc + bytes_output, "${field}: 0x${format_str}", ${args_str});
  assert(bytes_output < max_entry_length);
//::       if f_info["data_type"] == "uint32_t":
//::         format_str = " ".join(["%hhu" for i in xrange(byte_width)])
  bytes_output += sprintf(entry_desc + bytes_output, " (${format_str})", ${args_str});
  assert(bytes_output < max_entry_length);
//::       #endif
//::     #endif
  bytes_output += sprintf(entry_desc + bytes_output, ",\t");
  assert(bytes_output < max_entry_length);
  key += ${byte_width};
//::   #endfor
  bytes_output += sprintf(entry_desc + bytes_output, "\n");
  assert(bytes_output < max_entry_length);
//::   if match_type == "ternary":
  bytes_output += sprintf(entry_desc + bytes_output, "\tmask:\n\t\t");
  assert(bytes_output < max_entry_length);
  uint8_t *mask = entry->mask;
//::     for field, m_type in t_info["reordered_match_fields"]:
//::       if m_type == "valid":
//::         byte_width = 1
  bytes_output += sprintf(entry_desc + bytes_output, "${field}_valid: %x", (int) mask[0]);
  assert(bytes_output < max_entry_length);
//::       else:
//::         f_info = field_info[field]
//::         byte_width = f_info["byte_width_phv"]
//::         format_str = "%02x" * byte_width
//::         args_str = ", ".join(["mask[%d]" % i for i in xrange(byte_width)])
  bytes_output += sprintf(entry_desc + bytes_output, "${field}: 0x${format_str}", ${args_str});
  assert(bytes_output < max_entry_length);
//::       #endif
  bytes_output += sprintf(entry_desc + bytes_output, ",\t");
  assert(bytes_output < max_entry_length);
  mask += ${byte_width};
//::     #endfor
  bytes_output += sprintf(entry_desc + bytes_output, "\n");
  assert(bytes_output < max_entry_length);
//::   #endif
//::   if match_type == "ternary":
  bytes_output += sprintf(entry_desc + bytes_output, "\tpriority:\n\t\t");
  assert(bytes_output < max_entry_length);
  bytes_output += sprintf(entry_desc + bytes_output, "%d\n", entry->priority);
  assert(bytes_output < max_entry_length);
//::   #endif
//::   if match_type == "lpm":
  bytes_output += sprintf(entry_desc + bytes_output, "\tprefix_length:\n\t\t");
  assert(bytes_output < max_entry_length);
  bytes_output += sprintf(entry_desc + bytes_output, "%d\n", entry->prefix_width);
  assert(bytes_output < max_entry_length);
//::   #endif
  bytes_output += sprintf(entry_desc + bytes_output, "\taction:\n\t\t");
  assert(bytes_output < max_entry_length);
  ActionFn action = table->actions[entry->action_id];
  bytes_output += sprintf(entry_desc + bytes_output, "%s\n", action_get_name(action));
  assert(bytes_output < max_entry_length);
  bytes_output += sprintf(entry_desc + bytes_output, "\taction data:\n\t\t");
  assert(bytes_output < max_entry_length);
//::   if t_info["action_profile"] is None:
  action_dump_action_data(action, entry->action_data, entry_desc + bytes_output, max_entry_length);
//::   else:
  bytes_output += sprintf(entry_desc + bytes_output, "%02x%02x%02x%02x",
      entry->action_data[0], entry->action_data[1],
      entry->action_data[2], entry->action_data[3]);
  assert(bytes_output < max_entry_length);
//::   #endif
}

void
tables_${table}_get_entry_string(p4_pd_entry_hdl_t entry_hdl, char *entry_desc, int max_entry_length) {
  table_t *table = &tables_array[RMT_TABLE_${table}];
  pthread_rwlock_wrlock(&table->lock);

  _tables_${table}_get_entry_string(entry_hdl, entry_desc, max_entry_length);

  pthread_rwlock_unlock(&table->lock);
}

//:: #endfor
//:: for table_name in [t for t in table_info.keys() if table_info[t]['support_timeout']]:
static inline ${table_name}_entry_t *
${table_name}_get_entry(const table_t *table, const int entry_index) {
  if (table->max_size <= entry_index) {
    RMT_LOG(P4_LOG_LEVEL_WARN, "Entry index %d exceeds max table size %d in ${table_name}\n", entry_index, table->max_size);
    return NULL;
  }

  if (!test_index(&table->jarray_used, (Word_t)entry_index)) {
    RMT_LOG(P4_LOG_LEVEL_WARN, "Entry index %d not set in ${table_name}\n", entry_index);
    return NULL;
  }

  ${table_name}_entry_t *entry = (${table_name}_entry_t *)(table->entries_array);
  return entry + entry_index;
}

static void
${table_name}_sweep_entries(struct timeval *current_time) {
  table_t *table = &tables_array[RMT_TABLE_${table_name}];
  struct timeval zero = { .tv_sec = 0, .tv_usec = 0 };

  pthread_rwlock_rdlock(&table->lock);

  Word_t i = 0;
  int Rc_int;
  J1F(Rc_int, table->jarray_used, i);
  for(; Rc_int; ) {
    ${table_name}_entry_t *entry = ${table_name}_get_entry(table, (const int)i);
    if (1 == entry->is_hit_after_last_sweep) {
      entry->is_hit_after_last_sweep = (sig_atomic_t)0;
      gettimeofday(&(entry->last_hit_time), NULL);
    }
    // |entry->ttl| of zero indicates entry timeout is disabled on this entry.
    if (timercmp(&entry->ttl, &zero, !=)) {
      struct timeval expiry_time;
      timeradd(&entry->last_hit_time, &entry->ttl, &expiry_time);
      if (entry->is_reported_hit) {
        if (!timercmp(&expiry_time, current_time, >)) {
          RMT_LOG(P4_LOG_LEVEL_VERBOSE,
                  "Reporting entry %u timeout in ${table_name} table at %ld.%06ld\n",
                  (uint32_t)i, current_time->tv_sec, current_time->tv_usec);
          entry->is_reported_hit = (sig_atomic_t)0;
          p4_pd_notify_timeout_cb timeout_cb_fn = table->timeout_cb_fn;
          void *timeout_client_data = table->timeout_client_data;

          // Mark dirty to ensure that this entry is not modified or the buffer
          // space reused for a new entry.
          J1S(Rc_int, table->jarray_dirty, i);

          pthread_rwlock_unlock(&table->lock);

          if (NULL != timeout_cb_fn) {
            timeout_cb_fn((p4_pd_entry_hdl_t)i, timeout_client_data);
          }
          else {
            RMT_LOG(P4_LOG_LEVEL_WARN, "No TTL callback function for entry %d in ${table_name}\n", (int)i);
          }

          pthread_rwlock_rdlock(&table->lock);

          J1U(Rc_int, table->jarray_dirty, i);
        }
      }
      else {
        // The else block is executed if the entry was reported as expired.
        if (timercmp(&expiry_time, current_time, >)) {
          RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Entry %u hit in ${table_name} table\n", (uint32_t)i);
          entry->is_reported_hit = (sig_atomic_t)1;
          // We do not have a callback function to report entry hit.
        }
      }
    }
    J1N(Rc_int, table->jarray_used, i);
  }

  pthread_rwlock_unlock(&table->lock);
}

void
tables_${table_name}_get_hit_state(const int entry_index,
                                   bool *const is_hit) {
  table_t *table = &tables_array[RMT_TABLE_${table_name}];

  // Acquiring write lock because multiple clients might call this function
  // simultaneously. Only one of them should see the |*is_hit| true.
  pthread_rwlock_wrlock(&table->lock);

  ${table_name}_entry_t *entry = ${table_name}_get_entry(table, entry_index);
  if (1 == entry->is_hit) {
    entry->is_hit = (sig_atomic_t)0;
    *is_hit = true;
  }
  else {
    *is_hit = false;
  }
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
          "Reporting hit state %s for entry %d in ${table_name}\n",
          (*is_hit ? "true" : "false"), entry_index);

  pthread_rwlock_unlock(&table->lock);
}

p4_pd_status_t
tables_${table_name}_set_entry_ttl(const int entry_index, const uint32_t ttl) {
  table_t *table = &tables_array[RMT_TABLE_${table_name}];

  pthread_rwlock_wrlock(&table->lock);

  if (NULL == table->timeout_cb_fn) {
    RMT_LOG(P4_LOG_LEVEL_WARN, "Callback function in ${table_name} in NULL\n");
  }

  ${table_name}_entry_t *entry = ${table_name}_get_entry(table, entry_index);
  if (NULL == entry) {
    RMT_LOG(P4_LOG_LEVEL_ERROR, "Cannot find entry %d in ${table_name}\n", entry_index);
    pthread_rwlock_unlock(&table->lock);
    return 1;
  }

  struct timeval current_time;
  gettimeofday(&current_time, NULL);
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
          "Setting TTL %u ms for entry %d in ${table_name} at %ld.%06ld\n", ttl,
          entry_index, current_time.tv_sec, current_time.tv_usec);

  const struct timeval entry_ttl = {.tv_sec = ttl /1000,
                                    .tv_usec= (ttl % 1000) * 1000};
  entry->ttl = entry_ttl;
  entry->is_hit_after_last_sweep = (sig_atomic_t)0;
  entry->is_reported_hit = (sig_atomic_t)1;
  entry->last_hit_time = current_time;

  pthread_rwlock_unlock(&table->lock);

  return 0;
}

void
tables_${table_name}_enable_entry_timeout(p4_pd_notify_timeout_cb cb_fn,
                                          const uint32_t max_ttl,
                                          void *client_data) {
  table_t *table = &tables_array[RMT_TABLE_${table_name}];

  pthread_rwlock_wrlock(&table->lock);

  table->timeout_cb_fn = cb_fn;
  (void)max_ttl;
  table->timeout_client_data = client_data;

  RMT_LOG(P4_LOG_LEVEL_INFO, "Enabling entry timeout in ${table_name}\n");

  int Rc_int;
  Word_t index = 0;
  J1F(Rc_int, table->jarray_used, index);
  for (; Rc_int; ) {
    ${table_name}_entry_t *entry = ${table_name}_get_entry(table, (int)index);
    assert(NULL != entry);
    entry->is_reported_hit = (sig_atomic_t)1;
    gettimeofday(&(entry->last_hit_time), NULL);
    RMT_LOG(P4_LOG_LEVEL_VERBOSE,
            "Enabling timeout on entry %d in ${table_name} at %ld.%06ld\n",
            (int)index, entry->last_hit_time.tv_sec,
            entry->last_hit_time.tv_usec);
    J1N(Rc_int, table->jarray_used, index);
  }

  pthread_rwlock_unlock(&table->lock);
}
//:: #endfor

static void *
entry_aging_loop(void *arg) {
  while (1) {
    // Sleep time is set to 1 second since that is the minimum timeout interval.
    struct timeval sleep_time = { .tv_sec = 1, .tv_usec = 0 };

    struct timeval current_time;
    gettimeofday(&current_time, NULL);

//:: for table_name in [t for t in table_info.keys() if table_info[t]['support_timeout']]:
    ${table_name}_sweep_entries(&current_time);
//:: #endfor

    const int select_return = select(0, NULL, NULL, NULL, &sleep_time);
    if (-1 == select_return) {
      RMT_LOG(P4_LOG_LEVEL_ERROR, "Error %d: Select interrupted due to %s\n",
              errno, strerror(errno));
    }
  }

  return NULL;
}

void tables_init(void){
//:: for table, t_info in table_info.items():
  ${table}_init();
//:: #endfor

  pthread_create(&entry_aging_thread, NULL, entry_aging_loop, NULL);

#ifdef P4RMT_USE_TCAM_CACHE
  pthread_create(&ternary_cache_thread, NULL, ternary_cache_cleanup, NULL);
#endif
}

void tables_print(rmt_table_t table_index) {
  printf("Printing table %s\n", rmt_table_string[table_index]);
  table_t *table = &tables_array[table_index];

  pthread_rwlock_rdlock(&table->lock);

  Word_t index = 0;
  int Rc_int;
  J1F(Rc_int, table->jarray_used, index); // Judy1First()
  for(; Rc_int; ) {
    printf ("entry at %lu\n", index);
    J1N(Rc_int, table->jarray_used, index);
  }

  pthread_rwlock_unlock(&table->lock);
}
