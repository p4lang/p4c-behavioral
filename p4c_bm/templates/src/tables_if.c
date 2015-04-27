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

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include "tables_if.h"
#include "tables.h"

//:: rocker_p4_prefix = "rocker_" + p4_prefix + "_"
//:: for table, t_info in table_info.items():
//::   match_type = t_info["match_type"]
//::   key_width = t_info["key_byte_width"]
void ${p4_prefix}_tables_add_entry_${table}(void *entry) {
  ${table}_entry_t new_entry;
  /* Make sure that the P4 specified fields are at the beginning
   * of both entries
   */
  memcpy(&new_entry, entry, sizeof(${rocker_p4_prefix}${table}_entry_t));
  (void)tables_add_entry_${table}(&new_entry);
}

void ${p4_prefix}_tables_set_default_${table}(void *entry) {
  ${table}_entry_t new_entry;
  /* Make sure that the P4 specified fields are at the beginning
   * of both entries
   */
  memcpy(&new_entry, entry, sizeof(${rocker_p4_prefix}${table}_entry_t));
  tables_set_default_${table}(new_entry.action_id, new_entry.action_data);
}

void ${p4_prefix}_tables_print_entries_${table}() {
  // just call the table function directly
  tables_print_entries_${table}();
}
//:: #endfor


p4_rmt_table_ops_t ${p4_prefix}_table_ops[] = {
    {NULL, NULL, NULL, NULL, NULL}, /* table id 0 is not used */
//:: for table, t_info in table_info.items():
    {.add = & ${p4_prefix}_tables_add_entry_${table}, 
     .mod = NULL, 
     .del = NULL, 
     .default_action = &${p4_prefix}_tables_set_default_${table},
     .table_print = &${p4_prefix}_tables_print_entries_${table},
    },
//:: #endfor
};
