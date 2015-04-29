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
/*
* SAI auto generated header file
*/

#ifndef _SAI_TEMPL_H
#define _SAI_TEMPL_H

#include <p4_sim/saitypes.h>

//:: p4_pd_prefix = "p4_pd_" + p4_prefix + "_"

//:: def get_type(byte_width):
//::   if byte_width == 1:
//::     return "uint8_t"
//::   elif byte_width == 2:
//::     return "uint16_t"
//::   elif byte_width <= 4:
//::     return "uint32_t"
//::   else:
//::     return "uint8_t *"
//::   #endif
//:: #enddef
//::
//:: # match_fields is list of tuples (name, type)
//:: def gen_match_params(match_fields, field_info):
//::   params = []
//::   for field, type in match_fields:
//::     if type == "valid":
//::       params += [(field + "_valid", 1)]
//::       continue
//::     #endif
//::     f_info = field_info[field]
//::     bytes_needed = (f_info["bit_width"] + 7 ) / 8
//::     params += [(field, bytes_needed)]
//::     if type == "lpm": params += [(field + "_prefix_length", 2)]
//::     if type == "ternary": params += [(field + "_mask", bytes_needed)]
//::   #endfor
//::   return params
//:: #enddef
//::
//:: def gen_action_params(names, byte_widths, _get_type = get_type):
//::   params = []
//::   for name, width in zip(names, byte_widths):
//::     name = "action_" + name
//::     params += [(name, width)]
//::   #endfor
//::   return params
//:: #enddef
//::
//::

//:: for table, t_info in table_info.items():

//::   act_prof_name = t_info["action_profile"]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   if act_prof_name is not None:
//::     # Get the  actions associated with act_prof_name
//::     parent_table = table
//::     parent_actions = t_info["actions"]
//::     table = act_prof_name
//::     act_prof = action_profiles[act_prof_name]
//::     t_info["actions"] = act_prof["actions"]
//::   #endif

//::   if has_match_spec:
//::      match_params = gen_match_params(t_info["match_fields"], field_info)

/*
*   ${table.upper()} Table API
*/

/*
*  This module defines SAI ${table.upper()} API
*/
//::     if act_prof_name is not None:
typedef struct _sai_${table}_${parent_table}_t {
//::     else:
typedef struct _sai_${table}_entry_t {
//::     #endif
//::     for name, width in match_params:
//::       if width > 4:
  uint8_t ${name}[${width}];
//::       else:
//::         type_ = get_type(width)
  ${type_} ${name};
//::       #endif
//::     #endfor
//::     if act_prof_name is not None:
} sai_${table}_${parent_table}_t;


typedef struct _sai_${table}_entry_t {
//::       name = match_params[0][0]
//::       width = match_params[0][1]
//::       if width > 4:
  uint8_t ${name}[${width}];
//::       else:
//::         type_ = get_type(width)
  ${type_} ${name};
//::       #endif
} sai_${table}_entry_t;
//::     else:
} sai_${table}_entry_t;
//::     #endif
//::   #endif

/*
*  Attribute Id for sai ${table} object
*/

typedef enum  _sai_${table}__attr_t {
        SAI_${table.upper()}_ATTR_PACKET_ACTION,
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_match_spec:
//::       params += [p4_pd_prefix + table + "_match_spec_t *match_spec"]
//::     #endif
//::     if has_action_spec:
//::       for p in a_info["param_names"]:
        SAI_${table.upper()}_ATTR_${p.upper()},
//::       #endfor
//::     #endif
//::     if match_type == "ternary":
//::       params += ["int priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [p4_pd_prefix + action + "_action_spec_t *action_spec"]
//::     #endif
//::     if t_info["support_timeout"]:
//::       params += ["uint32_t ttl"]
//::     #endif
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + table + "_table_add_with_" + action
//::   #endfor
        SAI_${table.upper()}_ATTR_CUSTOM_RANGE_BASE  = 0x10000000
} sai_${table}_attr_t;

typedef sai_status_t (*sai_create_${table}_fn)(
//::   if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::   #endif
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list
    );

typedef sai_status_t (*sai_remove_${table}_fn)(
//::   if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry
//::   #endif
    );

typedef sai_status_t (*sai_set_${table}_attribute_fn)(
//::   if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::   #endif
    _In_ const sai_attribute_t *attr
    );

typedef sai_status_t (*sai_get_${table}_attribute_fn)(
//::   if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::   #endif
    _In_ uint32_t attr_count,
    _Inout_ const sai_attribute_t *attr_list
    );

typedef sai_status_t (*sai_remove_all_${table}s_fn)(void);

//::   if act_prof_name is not None:
/*
    Member add/delete functions for ${table}
*/
typedef sai_status_t (*sai_add_${parent_table}_to_${table}_fn)(
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ uint32_t ${parent_table}_count,
    _In_ const sai_${table}_${parent_table}_t* ${parent_table}_list
    );

typedef sai_status_t (*sai_remove_${parent_table}_from_${table}_fn)(
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ uint32_t ${parent_table}_count,
    _In_ const sai_${table}_${parent_table}_t* ${parent_table}_list
    );
//::   #endif

//::   if t_info["bytes_counter"] is not None or t_info["packets_counter"] is not None:
typedef enum _sai_${table}_stat_counter_t
{
//::   if t_info["bytes_counter"] is not None:
    SAI_${table.upper()}_STAT_BYTES
//::   #endif
//::   if t_info["packets_counter"] is not None:
    SAI_${table.upper()}_STAT_PACKETS
//::   #endif
} sai_${table}_stat_counter_t;

typedef sai_status_t (*sai_get_${table}_stats_fn)(
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ const sai_${table}_stat_counter_t *counter_ids,
    _In_ uint32_t number_of_counters,
    _Out_ uint64_t* counters
    );
//::   #endif

typedef struct _sai_${table}_api_t
{
    sai_create_${table}_fn                      create_${table};
    sai_remove_${table}_fn                      remove_${table};
    sai_set_${table}_attribute_fn               set_${table}_attribute;
    sai_get_${table}_attribute_fn               get_${table}_attribute;
//::   if act_prof_name is not None:
    sai_add_${parent_table}_to_${table}_fn  add_${parent_table}_to_${table};
    sai_remove_${parent_table}_from_${table}_fn remove_${parent_table}_from_${table};
//::   #endif
    sai_remove_all_${table}s_fn                 remove_all_${table}s;
//::   if t_info["bytes_counter"] is not None or t_info["packets_counter"] is not None:
    sai_get_${table}_stats_fn                   get_${table}_stats;
//::   #endif
} sai_${table}_api_t;

//:: #endfor

#endif // _SAI_TEMPL_H

