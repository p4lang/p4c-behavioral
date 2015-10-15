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

#include <p4_sim/pd.h>
#include "actions.h"
#include "action_profiles.h"
#include "tables.h"
#include "enums.h"
#include "value_set.h"
#include "lf.h"
#include "stateful.h"
#include "pd_ms.h"

#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define PD_DEBUG 1

#define HOST_BYTE_ORDER_CALLER 1

#define BYTE_ROUND_UP(x) ((x + 7) >> 3)

//:: p4_pd_prefix = "p4_pd_" + p4_prefix + "_"
//::
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
//:: def get_num_match_bits(match_fields, field_info):
//::   num_bits = 0
//::   for field, type in match_fields:
//::     if type == "valid": continue
//::     f_info = field_info[field]
//::     num_bits += f_info["bit_width"]
//::   #endfor
//::   return num_bits
//:: #enddef
//::
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
//:: def gen_action_params(names, byte_widths):
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
//::   if not t_info["match_fields"]: continue
static inline void build_key_${table}
(
 uint8_t *key,
 ${p4_pd_prefix}${table}_match_spec_t *match_spec
)
{
#ifdef HOST_BYTE_ORDER_CALLER
  uint32_t tmp32;
  (void) tmp32;
#endif

//::   for field_name, field_match_type in t_info["reordered_match_fields"]:
//::     if field_match_type == "valid":
  *(key++) = (match_spec->${field_name}_valid > 0);

//::       continue
//::     #endif
//::     f_info = field_info[field_name]
//::     width = (f_info["bit_width"] + 7 ) / 8
//::     if width > 4:
  memcpy(key, match_spec->${field_name}, ${width});
  key += ${width};
//::     else:
#ifdef HOST_BYTE_ORDER_CALLER
  tmp32 = match_spec->${field_name};
  tmp32 = htonl(tmp32);
  *(uint32_t *) key = tmp32;
#else
  *(uint32_t *) key = match_spec->${field_name};
#endif
  key += sizeof(uint32_t);
//::     #endif
  
//::   #endfor
}

//:: #endfor
//::
//::
//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]: continue
//::   match_type = t_info["match_type"]
//::   if match_type != "ternary" and match_type != "lpm": continue
/*static inline*/ void build_mask_${table}
(
 uint8_t *mask,
 ${p4_pd_prefix}${table}_match_spec_t *match_spec
)
{
#ifdef HOST_BYTE_ORDER_CALLER
  uint32_t tmp32;
  uint16_t tmp16;
  (void) tmp32;
  (void) tmp16;
#endif
  int bits_to_reset; /* for lpm mask */
  (void) bits_to_reset; /* compiler */

//::   for field_name, field_match_type in t_info["reordered_match_fields"]:
//::
//::     if field_match_type == "valid":
//::       # already set to 1s
  mask++;
//::       continue
//::     #endif
//::
//::     f_info = field_info[field_name]
//::     width = (f_info["bit_width"] + 7 ) / 8
//::     field_width = max(4, width)
//::     bit_width = f_info["bit_width"]
//::
//::     if field_match_type == "exact":
//::       # already set to 1s
  mask += ${field_width};
//::
//::     elif field_match_type == "ternary":
//::       if width > 4:
  memcpy(mask, match_spec->${field_name}_mask, ${width});
  mask += ${width};
//::       else:
#ifdef HOST_BYTE_ORDER_CALLER
  tmp32 = match_spec->${field_name}_mask;
  tmp32 = htonl(tmp32);
  *(uint32_t *) mask = tmp32;
#else
  *(uint32_t *) mask = match_spec->${field_name}_mask;
#endif
  mask += sizeof(uint32_t);
//::       #endif
//::
//::     elif field_match_type == "lpm":
  bits_to_reset = ${bit_width} - match_spec->${field_name}_prefix_length;
  if (bits_to_reset >= 8) {
    memset(mask + ${width} - bits_to_reset / 8, 0, bits_to_reset / 8);
  }
  if (bits_to_reset % 8 != 0) {
    mask[${width} - 1 - bits_to_reset / 8] = (unsigned char) 0xFF << (bits_to_reset % 8);
  }
  mask += ${field_width};
//::     #endif

//::   #endfor
}

//:: #endfor

//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]: continue
//::   byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::   action_params = gen_action_params(a_info["param_names"], byte_widths)
static inline void build_action_spec_${action}
(
 uint8_t *data,
 ${p4_pd_prefix}${action}_action_spec_t *action_spec
)
{
#ifdef HOST_BYTE_ORDER_CALLER
  uint16_t tmp16;
  uint32_t tmp32;
  (void) tmp16;
  (void) tmp32;
#endif
//::
//::   for idx, param in enumerate(action_params):

//::     name, width = param
//::     if width > 4:
  memcpy(data, action_spec->${name}, ${width});
  data += ${width};
//::     else:
#ifdef HOST_BYTE_ORDER_CALLER
  tmp32 = action_spec->${name};
  tmp32 = htonl(tmp32);
  *(uint32_t *) data = tmp32;
#else
  *(uint32_t *) data = action_spec->${name};
#endif
  data += sizeof(uint32_t);
//::     #endif

//::   #endfor

}

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
)
{
  ${table}_entry_t entry;

//:: if t_info["support_timeout"] is True:
  entry.ttl.tv_sec = ttl / 1000;
  entry.ttl.tv_usec = (ttl % 1000) * 1000;
//:: #endif

//::     action_idx = t_info["actions_idx"][action]
  entry.action_id = ${action_idx};

//::     if has_match_spec:
  build_key_${table}(entry.key, match_spec);
//::     #endif

//::     if match_type == "ternary":
  /* we start by setting the mask entirely to 1s */
  memset(entry.mask, 0xFF, sizeof(entry.mask));
  build_mask_${table}(entry.mask, match_spec);
//::     #endif

//::     if match_type == "lpm":
//::       lpm_field_name, _ = t_info["reordered_match_fields"][-1]
//::       f_info = field_info[lpm_field_name]
//::       width = (f_info["bit_width"] + 7) / 8
  entry.prefix_width = match_spec->${lpm_field_name}_prefix_length;
//::       # for container alignment
//::       if width <= 4:
  entry.prefix_width += 8 * sizeof(uint32_t) - ${f_info["bit_width"]};
//::       #endif
//::       exact_fields_width = 0 # will include valid matches
//::       for field_name, _ in t_info["reordered_match_fields"][:-1]:
//::         f_info = field_info[field_name]
//::         width = (f_info["bit_width"] + 7) / 8
//::         exact_fields_width += 8 * max(width, 4)
//::       #endfor
  entry.prefix_width += ${exact_fields_width};
//::     elif match_type == "ternary":
  entry.priority = priority;
//::     #endif

//::     if has_action_spec:
  build_action_spec_${action}(entry.action_data, action_spec);
//::     #endif

  int rv = tables_add_entry_${table}(&entry);
  *entry_hdl = rv;
  return (rv < 0);
}

//::   #endfor
//:: #endfor

/* DELETE ENTRIES */

//:: for table, t_info in table_info.items():
//::   name = p4_pd_prefix + table + "_table_delete"
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
)
{
  return tables_delete_entry_${table}(entry_hdl);
}

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
)
{
//::     action_data_width = t_info["action_data_byte_width"]
  uint8_t _data[${action_data_width}];

//::     if has_action_spec:
  build_action_spec_${action}(_data, action_spec);
//::     #endif

//::     action_idx = t_info["actions_idx"][action]
  return tables_modify_entry_${table}(entry_hdl, ${action_idx}, _data);
}

//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   for action in t_info["actions"]:
//::     name = p4_pd_prefix + table + "_set_default_action_" + action
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
)
{
//::     action_data_width = t_info["action_data_byte_width"]
  uint8_t _data[${action_data_width}];

//::     if has_action_spec:
  build_action_spec_${action}(_data, action_spec);
//::     #endif

//::     action_idx = t_info["actions_idx"][action]
  return tables_set_default_${table}(${action_idx}, _data);
}

//::   #endfor
//:: #endfor


/* INDIRECT ACTION DATA AND MATCH SELECT */

//:: _action_profiles = set()
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   if act_prof in _action_profiles: continue
//::   _action_profiles.add(act_prof)
//::   act_prof_info = action_profiles[act_prof]
//::   has_selector = act_prof_info["selection_key"] is not None
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
)
{
//::     action_data_width = act_prof_info["action_data_byte_width"]
  char _data[${action_data_width}];

//::     if has_action_spec:
  build_action_spec_${action}((uint8_t *) _data, action_spec);
//::     #endif

  *mbr_hdl = action_profiles_add_entry_${act_prof}(_data);

  p4_pd_ms_new_mbr(&ms_${act_prof}_state, 0, *mbr_hdl);
  p4_pd_ms_set_mbr_act(&ms_${act_prof}_state, 0,
		    *mbr_hdl, action_${action});
  
  return 0;
}

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
)
{
  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_mbr_act(&ms_${act_prof}_state, dev_id, mbr_hdl);
  // this target only supports modify with same action
  if(act_hdl != action_${action}) return -1;

//::     action_data_width = act_prof_info["action_data_byte_width"]
//::     if has_action_spec:
  char _data[${action_data_width}];
  build_action_spec_${action}((uint8_t *) _data, action_spec);
  return action_profiles_modify_entry_${act_prof}(mbr_hdl, _data);
//::     else:
  return 0;
//::     #endif
}

//::   #endfor
//::
//::   if has_selector:
void grp_del_mbr_fn_${act_prof}
(
 uint8_t dev_id,
 p4_pd_mbr_hdl_t mbr_hdl,
 p4_pd_grp_hdl_t grp_hdl,
 void *aux
)
{
  action_profiles_delete_member_from_group(RMT_ACT_PROF_${act_prof},
					   grp_hdl,
					   mbr_hdl);
}

//::   #endif
//::
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_member"
p4_pd_status_t
${name}
(
 ${param_str}
)
{
//::   if has_selector:
  /* check group membership */
  p4_pd_ms_mbr_apply_to_grps(&ms_${act_prof}_state, 0, mbr_hdl,
			  grp_del_mbr_fn_${act_prof}, NULL);
//::   #endif
  p4_pd_status_t status = action_profiles_delete_entry_${act_prof}(mbr_hdl);
  p4_pd_ms_del_mbr(&ms_${act_prof}_state, 0, mbr_hdl);
  return status;
}

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
)
{
  *grp_hdl = action_profiles_create_group(RMT_ACT_PROF_${act_prof});
  return 0;
}

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "uint8_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_group"
p4_pd_status_t
${name}
(
 ${param_str}
)
{
  p4_pd_ms_del_grp(&ms_${act_prof}_state, 0, grp_hdl);
  return action_profiles_delete_group(RMT_ACT_PROF_${act_prof}, grp_hdl);
}

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
)
{
  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_mbr_act(&ms_${act_prof}_state, dev_id, mbr_hdl);
  p4_pd_status_t status = action_profiles_add_member_to_group(RMT_ACT_PROF_${act_prof},
							      grp_hdl,
							      mbr_hdl);
  p4_pd_ms_add_mbr_to_grp(&ms_${act_prof}_state, 0, mbr_hdl, grp_hdl);
  p4_pd_ms_set_grp_act(&ms_${act_prof}_state, 0, grp_hdl, act_hdl);
  return status;
}

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
)
{
  p4_pd_status_t status = action_profiles_delete_member_from_group(RMT_ACT_PROF_${act_prof},
								grp_hdl,
								mbr_hdl);
  p4_pd_ms_del_mbr_from_grp(&ms_${act_prof}_state, 0, mbr_hdl, grp_hdl);
  return status;
}

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
)
{
  return ${p4_pd_prefix + act_prof}_del_member_from_group(sess_hdl, dev_id,
						       grp_hdl, mbr_hdl);
}

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
)
{
  return ${p4_pd_prefix + act_prof}_add_member_to_group(sess_hdl, dev_id,
						     grp_hdl, mbr_hdl);
}

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
)
{
  ${table}_entry_t entry;

//:: if t_info["support_timeout"] is True:
  entry.ttl.tv_sec = ttl / 1000;
  entry.ttl.tv_usec = (ttl % 1000) * 1000;
//:: #endif

  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_mbr_act(&ms_${act_prof}_state,
					   0, mbr_hdl);
  entry.action_id = tables_get_action_id(RMT_TABLE_${table}, act_hdl);

//::     if has_match_spec:
  build_key_${table}(entry.key, match_spec);
//::     #endif

//::     if match_type == "ternary":
  /* we start by setting the mask entirely to 1s */
  memset(entry.mask, 0xFF, sizeof(entry.mask));
  build_mask_${table}(entry.mask, match_spec);
//::     #endif

//::     if match_type == "lpm":
//::       lpm_field_name, _ = t_info["reordered_match_fields"][-1]
//::       f_info = field_info[lpm_field_name]
//::       width = (f_info["bit_width"] + 7) / 8
  entry.prefix_width = match_spec->${lpm_field_name}_prefix_length;
//::       # for container alignment
//::       if width <= 4:
  entry.prefix_width += 8 * sizeof(uint32_t) - ${f_info["bit_width"]};
//::       #endif
//::       exact_fields_width = 0 # will include valid matches
//::       for field_name, _ in t_info["reordered_match_fields"][:-1]:
//::         f_info = field_info[field_name]
//::         width = (f_info["bit_width"] + 7) / 8
//::         exact_fields_width += 8 * max(width, 4)
//::       #endfor
  entry.prefix_width += ${exact_fields_width};
//::     elif match_type == "ternary":
  entry.priority = priority;
//::     #endif

  *(uint32_t *) entry.action_data = mbr_hdl;
  assert(mbr_hdl >> 24 == 0);

  int rv = tables_add_entry_${table}(&entry);
  *entry_hdl = rv;
  return (rv < 0);
}

//::   if not has_selector: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_add_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
)
{
  ${table}_entry_t entry;

//:: if t_info["support_timeout"] is True:
  entry.ttl.tv_sec = ttl / 1000;
  entry.ttl.tv_usec = (ttl % 1000) * 1000;
//:: #endif

  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_grp_act(&ms_${act_prof}_state,
					   0, grp_hdl);
  entry.action_id = tables_get_action_id(RMT_TABLE_${table}, act_hdl);

//::     if has_match_spec:
  build_key_${table}(entry.key, match_spec);
//::     #endif

//::     if match_type == "ternary":
  /* we start by setting the mask entirely to 1s */
  memset(entry.mask, 0xFF, sizeof(entry.mask));
  build_mask_${table}(entry.mask, match_spec);
//::     #endif

//::     if match_type == "lpm":
//::       lpm_field_name, _ = t_info["reordered_match_fields"][-1]
//::       f_info = field_info[lpm_field_name]
//::       width = (f_info["bit_width"] + 7) / 8
  entry.prefix_width = match_spec->${lpm_field_name}_prefix_length;
//::       # for container alignment
//::       if width <= 4:
  entry.prefix_width += 8 * sizeof(uint32_t) - ${f_info["bit_width"]};
//::       #endif
//::       exact_fields_width = 0 # will include valid matches
//::       for field_name, _ in t_info["reordered_match_fields"][:-1]:
//::         f_info = field_info[field_name]
//::         width = (f_info["bit_width"] + 7) / 8
//::         exact_fields_width += 8 * max(width, 4)
//::       #endfor
  entry.prefix_width += ${exact_fields_width};
//::     elif match_type == "ternary":
  entry.priority = priority;
//::     #endif

  *(uint32_t *) entry.action_data = grp_hdl;
  assert(grp_hdl >> 24 == 0);
  entry.action_data[3] = 0x80;

  int rv = tables_add_entry_${table}(&entry);
  *entry_hdl = rv;
  return (rv < 0);
}

//:: #endfor

//:: for table, t_info in table_info.items():
//::   name = p4_pd_prefix + table + "_get_first_entry_handle"
p4_pd_status_t
${name}(p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int *index) {
  (*index) = tables_${table}_get_first_entry_handle();
  return 0;
}

//::   name = p4_pd_prefix + table + "_get_next_entry_handles"
p4_pd_status_t
${name}(p4_pd_sess_hdl_t sess_hdl, uint8_t dev_id, p4_pd_entry_hdl_t entry_handle, int n, int *next_entry_handles) {
  tables_${table}_get_next_entry_handles(entry_handle, n, next_entry_handles);
  return 0;
}

//::   name = p4_pd_prefix + table + "_get_entry"
p4_pd_status_t
${name}(p4_pd_sess_hdl_t sess_hdl, uint8_t dev_id, p4_pd_entry_hdl_t entry_hdl, char *entry_desc, int max_entry_length) {
  tables_${table}_get_entry_string(entry_hdl, entry_desc, max_entry_length);
  return 0;
}

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
)
{
  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_mbr_act(&ms_${act_prof}_state,
					   0, mbr_hdl);
  int action_id = tables_get_action_id(RMT_TABLE_${table}, act_hdl);

  assert(mbr_hdl >> 24 == 0);
  uint32_t _data = mbr_hdl;

  *entry_hdl = tables_set_default_${table}(action_id, (uint8_t *) &_data);
  return 0;
}

//::   if not has_selector: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl", "p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_set_default_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
)
{
  p4_pd_act_hdl_t act_hdl = p4_pd_ms_get_grp_act(&ms_${act_prof}_state,
					   0, grp_hdl);
  int action_id = tables_get_action_id(RMT_TABLE_${table}, act_hdl);

  assert(grp_hdl >> 24 == 0);
  uint32_t _data = grp_hdl & 0x80;

  *entry_hdl = tables_set_default_${table}(action_id, (uint8_t *) &_data);
  return 0;
}

//:: #endfor


/* END OF INDIRECT ACTION DATA AND MATCH SELECT */


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
)
{
  (void) flags;
  p4_pd_counter_value_t counter_value;
//::     c_name = counter
//::     if type_ == "packets" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_packets"
//::       #endif
  counter_value.packets = stateful_read_counter(&counter_${c_name}, entry_hdl);
//::     #endif
//::     if type_ == "bytes" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_bytes"
//::       #endif
  counter_value.bytes = stateful_read_counter(&counter_${c_name}, entry_hdl);
//::     #endif
  return counter_value;
}

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
)
{
//::     c_name = counter
//::     if type_ == "packets" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_packets"
//::       #endif
  stateful_write_counter(&counter_${c_name}, entry_hdl, counter_value.packets);
//::     #endif
//::     if type_ == "bytes" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_bytes"
//::       #endif
  stateful_write_counter(&counter_${c_name}, entry_hdl, counter_value.bytes);
//::     #endif
  return 0;
}

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
 )
{
  (void) flags;
  p4_pd_counter_value_t counter_value;
//::     c_name = counter
//::     if type_ == "packets" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_packets"
//::       #endif
  counter_value.packets = stateful_read_counter(&counter_${c_name}, index);
//::     #endif
//::     if type_ == "bytes" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_bytes"
//::       #endif
  counter_value.bytes = stateful_read_counter(&counter_${c_name}, index);
//::     #endif
  return counter_value;
}

//::     name = "p4_pd_" + p4_prefix + "_counter_write_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 p4_pd_counter_value_t counter_value
 )
{
//::     c_name = counter
//::     if type_ == "packets" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_packets"
//::       #endif
  stateful_write_counter(&counter_${c_name}, index, counter_value.packets);
//::     #endif
//::     if type_ == "bytes" or type_ == "packets_and_bytes":
//::       if type_ == "packets_and_bytes":
//::         c_name = counter + "_bytes"
//::       #endif
  stateful_write_counter(&counter_${c_name}, index, counter_value.bytes);
//::     #endif
  return 0;
}

//::   #endif
//:: #endfor

typedef struct {
  p4_pd_stat_sync_cb cb_fn;
  uint8_t dev_id;
  void *cb_cookie;
} sync_cb_arg_t;

static void *sync_cb(void *arg) {
  sync_cb_arg_t *cb_arg = (sync_cb_arg_t *) arg;
  cb_arg->cb_fn(cb_arg->dev_id, cb_arg->cb_cookie);
  free(cb_arg);
  return NULL;
}

//:: for counter, c_info in counter_info.items():
//::   name = "p4_pd_" + p4_prefix + "_counter_hw_sync_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_stat_sync_cb cb_fn,
 void *cb_cookie
 )
{
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  sync_cb_arg_t *cb_arg = (sync_cb_arg_t *) malloc(sizeof(sync_cb_arg_t));
  cb_arg->cb_fn = cb_fn;
  cb_arg->dev_id = dev_tgt.device_id;
  cb_arg->cb_cookie = cb_cookie;
  pthread_create(&thread, &attr, sync_cb, cb_arg);
  // I let thread go out of scope, which is okay, it is in detached state
  return 0;
}

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
)
{
  return tables_read_${type_}_counter_hit_${table}(); 
}

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
)
{
  return tables_read_${type_}_counter_miss_${table}();
}

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
static inline p4_pd_status_t configure_meter_${meter}
(
 ${param_str}
)
{
  p4_pd_status_t status = 1;
  double info_rate;
  uint32_t burst_size;

//::     if type_ == "packets":
  info_rate = (double) cir_pps / 1000000.;
  burst_size = cburst_pkts;
//::     else:
  info_rate = (double) cir_kbps / 1000000.;
  burst_size = cburst_kbits * 1000;
//::     #endif
  status |= stateful_meter_set_queue(&meter_${meter}, ${entry_or_index},
				     info_rate, burst_size,
				     METER_EXCEED_ACTION_COLOR_YELLOW);

//::     if type_ == "packets":
  info_rate = (double) pir_pps / 1000000.;
  burst_size = pburst_pkts;
//::     else:
  info_rate = (double) pir_kbps / 1000000.;
  burst_size = pburst_kbits * 1000;
//::     #endif
  status |= stateful_meter_set_queue(&meter_${meter}, ${entry_or_index},
				     info_rate, burst_size,
				     METER_EXCEED_ACTION_COLOR_RED);

  return status;
}

//::   
//::   if binding[0] == "direct":
//::     table = binding[1]
//::     name = "p4_pd_" + p4_prefix + "_" + table + "_configure_meter_entry"
p4_pd_status_t
${name}
(
 ${param_str}
)
{
//::     if type_ == "packets":
  return configure_meter_${meter}(sess_hdl, dev_tgt, ${entry_or_index},
				  cir_pps, cburst_pkts, pir_pps, pburst_pkts);
//::     else:
  return configure_meter_${meter}(sess_hdl, dev_tgt, ${entry_or_index},
				  cir_kbps, cburst_kbits, pir_kbps, pburst_kbits);
//::     #endif
}

//::   #endif
//::
//::   name = "p4_pd_" + p4_prefix + "_meter_configure_" + meter
p4_pd_status_t
${name}
(
 ${param_str}
)
{
//::     if type_ == "packets":
  return configure_meter_${meter}(sess_hdl, dev_tgt, ${entry_or_index},
				  cir_pps, cburst_pkts, pir_pps, pburst_pkts);
//::     else:
  return configure_meter_${meter}(sess_hdl, dev_tgt, ${entry_or_index},
				  cir_kbps, cburst_kbits, pir_kbps, pburst_kbits);
//::     #endif
}

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
//::   match_data = vs_info["match_data"]
//::   vs_match_params = gen_vs_match_params(match_data, field_info)
//::   param_str = ", ".join([get_type(p[1]) + " " + p[0] for p in vs_match_params])
static inline void build_value_set_entry_${vs}(uint8_t *value, uint8_t *mask, ${param_str}) {
  uint32_t tmp;

  (void)tmp; /* Compiler warning */
//::   for idx, param in enumerate(vs_match_params):
//::     name, width = param
//::     dest = "value" if idx % 2 == 0 else "mask"
//::     if width > 4:
  memcpy(${dest}, ${name}, ${width});
  ${dest} += ${width};
//::     else:
  tmp = ${name};
  *(uint32_t *) ${dest} = htonl(tmp);
  ${dest} += sizeof(uint32_t);
//::     #endif
//::   #endfor
}

//:: #endfor

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
)
{
//::   byte_width = vs_info["byte_width"]
  uint8_t value[${byte_width}];
  uint8_t mask[${byte_width}];
//::   build_key_args = ", ".join([p[0] for p in vs_match_params])
  build_value_set_entry_${vs}(value, mask, ${build_key_args});
  *value_hdl = value_set_${vs}_add(value, mask);
  return (*value_hdl < 0);
}

//:: #endfor

//:: for vs, vs_info in value_sets.items():
//::   name = "p4_pd_" + p4_prefix + "_" + vs + "_value_set_delete"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_value_hdl_t value_hdl
)
{
  return value_set_${vs}_delete(value_hdl);
}

//:: #endfor


/* Clean all state */
//:: name = "p4_pd_" + p4_prefix + "_clean_all"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
)
{
  lf_clean_all();
  stateful_clean_all();
  return tables_clean_all();
}

/* Clean all tables */
//:: name = "p4_pd_" + p4_prefix + "_tables_clean_all"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
)
{
  return tables_clean_all();
}

//:: p4_pd_prefix = ["p4_pd", p4_prefix]
//:: lf_prefix = ["lf", p4_prefix]
//:: p4_pd_set_learning_timeout = "_".join(p4_pd_prefix + ["set_learning_timeout"])
//:: lf_set_learning_timeout = "_".join(lf_prefix + ["set_learning_timeout"])
p4_pd_status_t
${p4_pd_set_learning_timeout}(p4_pd_sess_hdl_t sess_hdl, uint8_t dev_id, const long int usecs) {
  (void)sess_hdl;
  (void)dev_id;
  ${lf_set_learning_timeout}(usecs);

  return 0;
}

//:: for lq in learn_quanta:
p4_pd_status_t
${lq["register_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id,
 ${lq["cb_fn_type"]}      cb_fn,
 void                    *cb_fn_cookie
)
{
  return (p4_pd_status_t)${lq["lf_register_fn"]}(sess_hdl, cb_fn, cb_fn_cookie);
}

p4_pd_status_t
${lq["deregister_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id
)
{
  return (p4_pd_status_t)${lq["lf_deregister_fn"]}(sess_hdl);
}

p4_pd_status_t
${lq["notify_ack_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 ${lq["msg_type"]}       *msg
)
{
  return (p4_pd_status_t)${lq["lf_notify_ack_fn"]}(sess_hdl, msg);
}
//:: #endfor

//:: for table_name in [t for t in table_info.keys() if table_info[t]['support_timeout']]:
//::   p4_pd_enable_hit_state_scan = "_".join(p4_pd_prefix + [table_name, "enable_hit_state_scan"])
//::   p4_pd_get_hit_state = "_".join(p4_pd_prefix + [table_name, "get_hit_state"])
//::   p4_pd_set_entry_ttl = "_".join(p4_pd_prefix + [table_name, "set_entry_ttl"])
//::   p4_pd_enable_entry_timeout = "_".join(p4_pd_prefix + [table_name, "enable_entry_timeout"])
p4_pd_status_t
${p4_pd_enable_hit_state_scan}(p4_pd_sess_hdl_t sess_hdl, uint32_t scan_interval) {
  // This function is a no-op. Needed for real hardware.
  (void)sess_hdl;
  (void)scan_interval;
  return 0;
}

p4_pd_status_t
${p4_pd_get_hit_state}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, p4_pd_hit_state_t *hit_state) {
  bool tables_is_hit;
  tables_${table_name}_get_hit_state(entry_hdl, &tables_is_hit);
  if (tables_is_hit) {
    *hit_state = ENTRY_HIT;
  }
  else {
    *hit_state = ENTRY_IDLE;
  }

  return 0;
}

p4_pd_status_t
${p4_pd_set_entry_ttl}(p4_pd_sess_hdl_t sess_hdl, p4_pd_entry_hdl_t entry_hdl, uint32_t ttl) {
  return tables_${table_name}_set_entry_ttl(entry_hdl, ttl);
}

p4_pd_status_t
${p4_pd_enable_entry_timeout}(p4_pd_sess_hdl_t sess_hdl,
                           p4_pd_notify_timeout_cb cb_fn,
                           uint32_t max_ttl,
                           void *client_data) {
  tables_${table_name}_enable_entry_timeout(cb_fn, max_ttl, client_data);
  return 0;
}
//:: #endfor
