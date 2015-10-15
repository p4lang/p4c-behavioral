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

#ifndef _RMT_PD_H
#define _RMT_PD_H

#include <stdint.h>

#include <p4_sim/pd_static.h>

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

/* MATCH STRUCTS */

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
/* ${table} has no match fields */

//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
typedef struct ${p4_pd_prefix}${table}_match_spec {
//::   for name, width in match_params:
//::     if width > 4:
  uint8_t ${name}[${width}];
//::     else:
//::       type_ = get_type(width)
  ${type_} ${name};
//::     #endif
//::   #endfor
} ${p4_pd_prefix}${table}_match_spec_t;

//:: #endfor


/* ACTION STRUCTS */

//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
/* ${action} has no parameters */

//::     continue
//::   #endif
//::   byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::   action_params = gen_action_params(a_info["param_names"], byte_widths)
typedef struct ${p4_pd_prefix}${action}_action_spec {
//::   for name, width in action_params:
//::     if width > 4:
  uint8_t ${name}[${width}];
//::     else:
//::       type_ = get_type(width)
  ${type_} ${name};
//::     #endif
//::   #endfor
} ${p4_pd_prefix}${action}_action_spec_t;

//:: #endfor


/* ADD ENTRIES */

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_match_spec:
//::       params += [p4_pd_prefix + table + "_match_spec_t *match_spec"]
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
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
//::     if has_match_spec:
 * @param match_spec
//::     #endif
//::     if match_type == "ternary":
 * @param priority
//::     #endif
//::     if has_action_spec:
 * @param action_spec
//::     #endif
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor

/* DELETE ENTRIES */

//:: for table, t_info in table_info.items():
//::   name = "p4_pd_" + p4_prefix + "_" + table + "_table_delete"
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_id
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 p4_pd_entry_hdl_t entry_hdl
);

//:: #endfor


/* MODIFY ENTRIES */

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_entry_hdl_t entry_hdl"]
//::     if has_action_spec:
//::       params += [p4_pd_prefix + action + "_action_spec_t *action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + table + "_table_modify_with_" + action
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_id
 * @param entry_hdl
//::     if has_action_spec:
 * @param action_spec
//::     #endif
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   for action in t_info["actions"]:
//::     name = "p4_pd_" + p4_prefix + "_" + table + "_set_default_action_" + action
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [p4_pd_prefix + action + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
//::     if has_action_spec:
 * @param action_spec
//::     #endif
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor


/* INDIRECT ACTION DATA AND MATCH SELECT */

//:: _action_profiles = set()
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   if act_prof in _action_profiles: continue
//::   _action_profiles.add(act_prof)
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += [p4_pd_prefix + action + "_action_spec_t *action_spec"]
//::     #endif
//::     params += ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + act_prof + "_add_member_with_" + action
p4_pd_status_t
${name}
(
 ${param_str}
);

//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t dev_id",
//::               "p4_pd_mbr_hdl_t mbr_hdl"]
//::     if has_action_spec:
//::       params += [p4_pd_prefix + action + "_action_spec_t *action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + act_prof + "_modify_member_with_" + action
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   if not has_selector: continue
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "uint16_t max_grp_size",
//::             "p4_pd_grp_hdl_t *grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_create_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_add_member_to_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_member_from_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_deactivate_group_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_reactivate_group_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if has_match_spec:
//::     params += [p4_pd_prefix + table + "_match_spec_t *match_spec"]
//::   #endif
//::   if match_type == "ternary":
//::     params += ["int priority"]
//::   #endif
//::   params_wo = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_wo)
//::   name = p4_pd_prefix + table + "_add_entry"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   if not has_selector: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_add_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   name = p4_pd_prefix + table + "_get_first_entry_handle"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int *index
);

//::   name = p4_pd_prefix + table + "_get_next_entry_handles"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 p4_pd_entry_hdl_t entry_handle,
 int n,
 int *next_entry_handles
);

//::   name = p4_pd_prefix + table + "_get_entry"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 uint8_t dev_id,
 p4_pd_entry_hdl_t entry_handle,
 char *entry_desc,
 int max_entry_length
);
//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_entry_hdl_t entry_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + table + "_table_delete"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   params_wo = params + ["p4_pd_mbr_hdl_t mbr_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_wo)
//::   name = p4_pd_prefix + table + "_set_default_entry"
p4_pd_status_t
${name}
(
 ${param_str}
);


//::   if not has_selector: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_set_default_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

/* END OF INDIRECT */

/* COUNTERS */

//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "p4_pd_" + p4_prefix + "_counter_read_" + counter
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
 * @param entry_hdl
 * @param flags
*/
p4_pd_counter_value_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 int flags
);

//::     name = "p4_pd_" + p4_prefix + "_counter_write_" + counter
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
 * @param entry_hdl
 * @param counter_value
*/
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 p4_pd_counter_value_t counter_value
);

//::   else:
//::     name = "p4_pd_" + p4_prefix + "_counter_read_" + counter
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
 * @param index
 * @param flags
*/
p4_pd_counter_value_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 int flags
);

//::     name = "p4_pd_" + p4_prefix + "_counter_write_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 p4_pd_counter_value_t counter_value
);

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "p4_pd_" + p4_prefix + "_counter_hw_sync_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_stat_sync_cb cb_fn,
 void *cb_cookie
);

//:: #endfor

/* GLOBAL TABLE COUNTERS */

//:: counter_types = ["bytes", "packets"]
//:: for type_ in counter_types:
//::   for table, t_info in table_info.items():
//::
//::       # read counter hit table
//::       name = "p4_pd_" + p4_prefix + "_" + table + "_table_read_" + type_ + "_counter_hit"
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
*/
uint64_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

//::
//::       # read counter miss table
//::       name = "p4_pd_" + p4_prefix + "_" + table + "_table_read_" + type_ + "_counter_miss"
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
*/
uint64_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

//::   #endfor
//:: #endfor


/* METERS */

//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if binding[0] == "direct":
//::     entry_or_index = "entry_hdl";
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     entry_or_index = "index";
//::     params += ["int index"]
//::   #endif
//::   if type_ == "packets":
//::     params += ["uint32_t cir_pps", "uint32_t cburst_pkts",
//::                "uint32_t pir_pps", "uint32_t pburst_pkts"]
//::   else:
//::     params += ["uint32_t cir_kbps", "uint32_t cburst_kbits",
//::                "uint32_t pir_kbps", "uint32_t pburst_kbits"]
//::   #endif
//::   param_str = ",\n ".join(params)
//::   
//::   if binding[0] == "direct":
//::     table = binding[1]
//::     name = "p4_pd_" + p4_prefix + "_" + table + "_configure_meter_entry"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endif
//::
//::   name = "p4_pd_" + p4_prefix + "_meter_configure_" + meter
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor


/* VALUE SETS */

//:: def gen_vs_match_params(field_or_current_list, field_info):
//::   params = []
//::   current_cnt = 0
//::   for type, ref in field_or_current_list:
//::     if type == "field_ref":
//::       name = ref
//::       f_info = field_info[ref]
//::       bytes_needed = (f_info["bit_width"] + 7) / 8
//::     elif type == "current":
//::       name = "current_" + str(current_cnt)
//::       current_cnt += 1
//::       bytes_needed = (ref[1] + 7) / 8
//::     #endif
//::     params += [(name, bytes_needed)]
//::     params += [(name + "_mask", bytes_needed)]
//::   #endfor
//::   return params
//:: #enddef
//::
//::
//:: for vs, vs_info in value_sets.items():
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   match_data = vs_info["match_data"]
//::   vs_match_params = gen_vs_match_params(match_data, field_info)
//::   params += [get_type(p[1]) + " " + p[0] for p in vs_match_params]
//::   params += ["p4_pd_value_hdl_t *value_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = "p4_pd_" + p4_prefix + "_" + vs + "_value_set_add"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: for vs, vs_info in value_sets.items():
//::   name = "p4_pd_" + p4_prefix + "_" + vs + "_value_set_delete"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_value_hdl_t value_hdl
);

//:: #endfor


/* Clean all state */
//:: name = "p4_pd_" + p4_prefix + "_clean_all"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

/* Clean all tables */
//:: name = "p4_pd_" + p4_prefix + "_tables_clean_all"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

//:: p4_pd_set_learning_timeout = p4_pd_prefix + "set_learning_timeout"
p4_pd_status_t
${p4_pd_set_learning_timeout}(p4_pd_sess_hdl_t sess_hdl, uint8_t dev_id, const long int usecs);

//:: for lq in learn_quanta:
typedef struct ${p4_pd_prefix}${lq["name"]}_digest_entry {
//:: for field in lq["fields"].keys():
//::   byte_width = ( field_info[field]["bit_width"] + 7 ) / 8
//::   if byte_width > 4:
  uint8_t ${field}[${byte_width}];
//::   else:
//::     type = get_type(byte_width)
  ${type} ${field};
//::   #endif
//:: #endfor
} ${lq["entry_type"]};

// Should be able to cast this to pipe_flow_lrn_msg_t.
typedef struct  ${p4_pd_prefix}${lq["name"]}_digest_msg {
  p4_pd_dev_target_t      dev_tgt;
  uint16_t                num_entries;
  ${lq["entry_type"]}    *entries;
} ${lq["msg_type"]};

// Should be able to cast this to pipe_flow_lrn_notify_cb.
typedef p4_pd_status_t (*${lq["cb_fn_type"]})(p4_pd_sess_hdl_t sess_hdl,
                                              ${lq["msg_type"]} *msg,
                                              void *callback_fn_cookie);

p4_pd_status_t
${lq["register_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id,
 ${lq["cb_fn_type"]}      cb_fn,
 void                    *cb_fn_cookie
);

p4_pd_status_t
${lq["deregister_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id
);

p4_pd_status_t
${lq["notify_ack_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 ${lq["msg_type"]}        *msg
);
//:: #endfor

//:: p4_pd_prefix = ["p4_pd", p4_prefix]

typedef enum {ENTRY_IDLE, ENTRY_HIT} p4_pd_hit_state_t;

typedef void (*p4_pd_notify_timeout_cb) (p4_pd_entry_hdl_t entry_hdl, void *client_data);

//:: for table_name in [t for t in table_info.keys() if table_info[t]['support_timeout']]:
//::   p4_pd_enable_hit_state_scan = "_".join(p4_pd_prefix + [table_name, "enable_hit_state_scan"])
//::   p4_pd_get_hit_state = "_".join(p4_pd_prefix + [table_name, "get_hit_state"])
//::   p4_pd_set_entry_ttl = "_".join(p4_pd_prefix + [table_name, "set_entry_ttl"])
//::   p4_pd_enable_entry_timeout = "_".join(p4_pd_prefix + [table_name, "enable_entry_timeout"])
p4_pd_status_t
${p4_pd_enable_hit_state_scan}(p4_pd_sess_hdl_t sess_hdl, uint32_t scan_interval);

p4_pd_status_t
${p4_pd_get_hit_state}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, p4_pd_hit_state_t *hit_state);

p4_pd_status_t
${p4_pd_set_entry_ttl}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, uint32_t ttl);

p4_pd_status_t
${p4_pd_enable_entry_timeout}(p4_pd_sess_hdl_t sess_hdl,
			      p4_pd_notify_timeout_cb cb_fn,
			      uint32_t max_ttl,
			      void *client_data);
//:: #endfor
#endif
