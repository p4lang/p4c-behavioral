# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

//:: # P4 Thrift RPC Input Template
# P4 Thrift RPC Input

//:: api_prefix = p4_prefix + "_"

include "res.thrift"

namespace py p4_pd_rpc
namespace cpp p4_pd_rpc
namespace c_glib p4_pd_rpc

typedef i32 EntryHandle_t
typedef i32 MemberHandle_t
typedef i32 GroupHandle_t
typedef binary MacAddr_t
typedef binary IPv6_t

struct ${api_prefix}counter_value_t {
  1: required i64 packets;
  2: required i64 bytes;
}


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
//:: def gen_action_params(names, byte_widths, _get_thrift_type = get_thrift_type):
//::   params = []
//::   for name, width in zip(names, byte_widths):
//::     name = "action_" + name
//::     params += [(name, width)]
//::   #endfor
//::   return params
//:: #enddef
//::
//::

struct ${api_prefix}counter_flags_t {
  1: required bool read_hw_sync;
}

# Match structs

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
struct ${api_prefix}${table}_match_spec_t {
//::   id = 1
//::   for name, width in match_params:
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::   id += 1
//::   #endfor
}

//:: #endfor


# Action structs

//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
//::     continue
//::   #endif
//::   byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::   action_params = gen_action_params(a_info["param_names"], byte_widths)
struct ${api_prefix}${action}_action_spec_t {
//::   id = 1
//::   for name, width in action_params:
//::     type_ = get_thrift_type(width)
  ${id}: required ${type_} ${name};
//::     id += 1
//::   #endfor
}

//:: #endfor

//:: for lq in learn_quanta:
//:: rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
//:: rpc_entry_type = api_prefix + lq["name"] + "_digest_entry_t"
struct ${rpc_entry_type} {
//::   fields = lq["fields"].keys()
//::   count = 1
//::   for field in fields:
//::     bit_width = field_info[field]["bit_width"]
//::     byte_width = (bit_width + 7 ) / 8
//::     if byte_width > 4:
//::       field_definition = "list<byte> %s" % field
//::     else:
//::       field_definition = "%s %s" % (get_thrift_type(byte_width), field)
//::     #endif
  ${count}: required ${field_definition};
//::   count += 1
//::   #endfor
}

struct ${rpc_msg_type} {
  1: required res.DevTarget_t             dev_tgt;
  2: required list<${rpc_entry_type}> msg;
  3: required i64                     msg_ptr;
}
//:: #endfor

service ${p4_prefix} {
    # Table entry add functions
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_match_spec:
//::       params += [api_prefix + table + "_match_spec_t match_spec"]
//::     #endif
//::     if match_type == "ternary":
//::       params += ["i32 priority"]
//::     #endif
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     if t_info["support_timeout"]:
//::       params += ["i32 ttl"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_table_add_with_" + action
    EntryHandle_t ${name}(${param_str});
//::   #endfor
//:: #endfor

    # Table entry modify functions
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   match_type = t_info["match_type"]
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::               "EntryHandle_t entry"]
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = table + "_table_modify_with_" + action
    i32 ${name}(${param_str});
//::   #endfor
//:: #endfor

    # Table entry delete functions
//:: for table, t_info in table_info.items():
//::   name = table + "_table_delete"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str});
//:: #endfor

    # Get first entry handle functions
//:: for table, t_info in table_info.items():
//::   name = table + "_get_first_entry_handle"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    i32 ${name}(${param_str});
//::   name = table + "_get_next_entry_handles"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry_hdl",
//::             "i32 n"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    list<i32> ${name}(${param_str});
//::   name = table + "_get_entry"
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "EntryHandle_t entry_hdl"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
    binary ${name}(${param_str});
//:: #endfor

    # Table set default action functions

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   for action in t_info["actions"]:
//::     name = table + "_set_default_action_" + action
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
    EntryHandle_t ${name}(${param_str});
//::   #endfor
//:: #endfor

    # INDIRECT ACTION DATA AND MATCH SELECT

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
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "res.DevTarget_t dev_tgt"]
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_add_member_with_" + action
    MemberHandle_t ${name}(${param_str});
//::     params = ["res.SessionHandle_t sess_hdl",
//::               "byte dev_id",
//::		   "MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += [api_prefix + action + "_action_spec_t action_spec"]
//::     #endif
//::     param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::     param_str = ", ".join(param_list)
//::     name = act_prof + "_modify_member_with_" + action
    i32 ${name}(${param_str});
//::   #endfor

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_member"
    i32 ${name}(${param_str});

//::   if not has_selector: continue
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt",
//::             "i16 max_grp_size"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_create_group"
    GroupHandle_t ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_add_member_to_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_del_member_from_group"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_deactivate_group_member"
    i32 ${name}(${param_str});

//::   params = ["res.SessionHandle_t sess_hdl",
//::             "byte dev_id",
//::             "GroupHandle_t grp",
//::             "MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   name = act_prof + "_reactivate_group_member"
    i32 ${name}(${param_str});

//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   has_action_spec = len(a_info["param_names"]) > 0
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   if has_match_spec:
//::     params += [api_prefix + table + "_match_spec_t match_spec"]
//::   #endif
//::   if match_type == "ternary":
//::     params += ["i32 priority"]
//::   #endif
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_add_entry"
    EntryHandle_t ${name}(${param_str});
//::
//::   if not has_selector: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_add_entry_with_selector"
    EntryHandle_t ${name}(${param_str});
//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   params_wo = params + ["MemberHandle_t mbr"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_wo)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_set_default_entry"
    EntryHandle_t ${name}(${param_str});
//::
//::   if not has_selector: continue
//::   params_w = params + ["GroupHandle_t grp"]
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params_w)]
//::   param_str = ", ".join(param_list)
//::   name = table + "_set_default_entry_with_selector"
    EntryHandle_t ${name}(${param_str});
//:: #endfor



    # clean all state
//:: name = "clean_all"
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);

    # clean table state
//:: name = "tables_clean_all"
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);

    # counters

//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "counter_read_" + counter
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl,
    2:res.DevTarget_t dev_tgt, 3:EntryHandle_t entry,
    4:${api_prefix}counter_flags_t flags);
//::     name = "counter_write_" + counter
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt,
    3:EntryHandle_t entry, 4:${api_prefix}counter_value_t counter_value);

//::   else:
//::     name = "counter_read_" + counter
    ${api_prefix}counter_value_t ${name}(1:res.SessionHandle_t sess_hdl,
    2:res.DevTarget_t dev_tgt, 3:i32 index, 4:${api_prefix}counter_flags_t
    flags);
//::     name = "counter_write_" + counter
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32
    index, 4:${api_prefix}counter_value_t counter_value);

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "counter_hw_sync_" + counter
    i32 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);
//:: #endfor

    # global table counters

//:: counter_types = ["bytes", "packets"]
//:: for type_ in counter_types:
//::   for table, t_info in table_info.items():
//::
//::     # read counter hit table
//::     name = table + "_table_read_" + type_ + "_counter_hit"
    i64 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);
//::
//::     # read counter miss table
//::     name = table + "_table_read_" + type_ + "_counter_miss"
    i64 ${name}(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt);
//::   #endfor
//:: #endfor

    # meters

//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   params = ["res.SessionHandle_t sess_hdl",
//::             "res.DevTarget_t dev_tgt"]
//::   if binding[0] == "direct":
//::     entry_or_index = "entry";
//::     params += ["EntryHandle_t entry"]
//::   else:
//::     entry_or_index = "index";
//::     params += ["i32 index"]
//::   #endif
//::   if type_ == "packets":
//::     params += ["i32 cir_pps", "i32 cburst_pkts",
//::                "i32 pir_pps", "i32 pburst_pkts"]
//::   else:
//::     params += ["i32 cir_kbps", "i32 cburst_kbits",
//::                "i32 pir_kbps", "i32 pburst_kbits"]
//::   #endif
//::   param_list = [str(count + 1) + ":" + p for count, p in enumerate(params)]
//::   param_str = ", ".join(param_list)
//::   
//::   if binding[0] == "direct":
//::     table = binding[1]
//::     name = table + "_configure_meter_entry"
    i32 ${name}(${param_str});

//::   #endif
//::
//::   name = "meter_configure_" + meter
    i32 ${name}(${param_str});

//:: #endfor

    # mirroring api

    i32 mirroring_mapping_add(1:i32 mirror_id, 2:i32 egress_port);
    i32 mirroring_mapping_delete(1:i32 mirror_id);
    i32 mirroring_mapping_get_egress_port(1:i32 mirror_id);

    void set_learning_timeout(1: res.SessionHandle_t sess_hdl, 2: byte dev_id, 3: i32 msecs);

//:: for lq in learn_quanta:
//::   rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
    void ${lq["name"]}_register(1: res.SessionHandle_t sess_hdl, 2: byte dev_id);
    void ${lq["name"]}_deregister(1: res.SessionHandle_t sess_hdl, 2: byte dev_id);
    ${rpc_msg_type} ${lq["name"]}_get_digest(1: res.SessionHandle_t sess_hdl);
    void ${lq["name"]}_digest_notify_ack(1: res.SessionHandle_t sess_hdl, 2: i64 msg_ptr);
//:: #endfor
}
