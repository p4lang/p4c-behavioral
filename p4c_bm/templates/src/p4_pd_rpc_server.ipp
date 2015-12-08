//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include "${p4_prefix}.h"

#include <iostream>

#include <string.h>

extern "C" {
#include <p4_sim/pd_static.h>
#include <p4_sim/pd.h>
#include <p4_sim/mirroring.h>
#include <p4_sim/pg.h>
#include <p4_sim/traffic_manager.h>
#include <p4utils/circular_buffer.h>
}

#include <list>
#include <map>
#include <pthread.h>

using namespace  ::p4_pd_rpc;
using namespace  ::res_pd_rpc;

class ${p4_prefix}Handler : virtual public ${p4_prefix}If {
public:
    ${p4_prefix}Handler() {
        // Your initialization goes here
//:: for lq in learn_quanta:
        pthread_mutex_init(&${lq["name"]}_mutex, NULL);
//:: #endfor
    }

//:: def get_type(byte_width):
//::   if byte_width == 1:
//::     return "const int8_t"
//::   elif byte_width == 2:
//::     return "const int16_t"
//::   elif byte_width <= 4:
//::     return "const int32_t"
//::   else:
//::     return "const std::string&"
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
//:: def gen_pd_args(params):
//::     # Need to coerce strings to appropriate type
//::     param_name_array = []
//::     for p in params:
//::         name = p[0]
//::         if p[1] > 4:
//::             name = "(uint8_t *)" + name + ".c_str()"
//::         #endif
//::         param_name_array.append(name)
//::     #endfor
//::     return param_name_array
//:: #enddef

    // Table entry add functions

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_match_spec:
//::       params += ["const " + api_prefix + table + "_match_spec_t &match_spec"]
//::     #endif
//::     if match_type == "ternary":
//::       params += ["const int32_t priority"]
//::     #endif
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     if t_info["support_timeout"] is True:
//::       params += ["const int32_t ttl"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = table + "_table_add_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::       match_params = gen_match_params(t_info["match_fields"], field_info)
//::       for name, width in match_params:
//::         if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::         else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::       action_params = gen_action_params(a_info["param_names"], byte_widths)
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_match_spec:
//::       pd_params += ["&pd_match_spec"]
//::     #endif
//::     if match_type == "ternary":
//::       pd_params += ["priority"]
//::     #endif
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     if t_info["support_timeout"] is True:
//::       pd_params += ["(uint32_t)ttl"]
//::     #endif
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//::   #endfor
//:: #endfor


    // Table entry modify functions

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   match_type = t_info["match_type"]
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const EntryHandle_t entry"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = table + "_table_modify_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::       action_params = gen_action_params(a_info["param_names"], byte_widths)
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif

//::     pd_params = ["sess_hdl", "dev_id", "entry"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});
    }

//::   #endfor
//:: #endfor


    // Table entry delete functions

//:: for table, t_info in table_info.items():
//::   name = table + "_table_delete"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, entry);
    }

//:: #endfor
//:: for table, t_info in table_info.items():
//::   name = table + "_get_first_entry_handle"
//::   pd_name = pd_prefix + name
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   param_str = ", ".join(params)
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        int32_t index = -1;

        ${pd_name}(sess_hdl, pd_dev_tgt, &index);
        return index;
    }

//::   name = table + "_get_next_entry_handles"
//::   pd_name = pd_prefix + name
//::   params = ["std::vector<int32_t> &next_entry_handles",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry_hdl",
//::             "const int32_t n"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        int *next_entry_handles_ptr = new int[n];
        ${pd_name}(sess_hdl, dev_id, entry_hdl, n, next_entry_handles_ptr);
        for (int i = 0; i < n && next_entry_handles_ptr[i] >= 0; ++i) {
          next_entry_handles.push_back(next_entry_handles_ptr[i]);
        }
        delete[] next_entry_handles_ptr;
    }

//::   name = table + "_get_entry"
//::   pd_name = pd_prefix + name
//::   params = ["std::string &entry_str",
//::             "const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const EntryHandle_t entry_hdl"]
//::   param_str = ", ".join(params)
    void ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        char entry[4096];
        memset(entry, 0, 4096);
        ${pd_name}((p4_pd_sess_hdl_t)sess_hdl, dev_id, (p4_pd_entry_hdl_t)entry_hdl, entry, 4096);
        entry_str = std::string(entry, std::find(entry, entry + 4096, '\0'));
    }

//:: #endfor


    // set default action

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is not None: continue
//::   for action in t_info["actions"]:
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = table + "_set_default_action_" + action
//::     pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::       action_params = gen_action_params(a_info["param_names"], byte_widths)
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_entry_hdl_t pd_entry;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_params += ["&pd_entry"]
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});

        // return pd_entry;
    }

//::   #endfor
//:: #endfor

//:: name = "clean_all"
//:: pd_name = pd_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
  }

//:: name = "tables_clean_all"
//:: pd_name = pd_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
  }

    // INDIRECT ACTION DATA AND MATCH SELECT

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
//::     params = ["const SessionHandle_t sess_hdl",
//::               "const DevTarget_t &dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof + "_add_member_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::       action_params = gen_action_params(a_info["param_names"], byte_widths)
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif
        p4_pd_mbr_hdl_t pd_mbr_hdl;

//::     pd_params = ["sess_hdl", "pd_dev_tgt"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_params += ["&pd_mbr_hdl"]
//::     pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_mbr_hdl;
    }

//::     params = ["const SessionHandle_t sess_hdl",
//::               "const int8_t dev_id",
//::               "const MemberHandle_t mbr"]
//::     if has_action_spec:
//::       params += ["const " + api_prefix + action + "_action_spec_t &action_spec"]
//::     #endif
//::     param_str = ", ".join(params)
//::     name = act_prof + "_modify_member_with_" + action
//::     pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

//::     if has_action_spec:
        ${pd_prefix}${action}_action_spec_t pd_action_spec;
//::       byte_widths = [(bw + 7) / 8 for bw in a_info["param_bit_widths"]]
//::       action_params = gen_action_params(a_info["param_names"], byte_widths)
//::       for name, width in action_params:
//::         if width <= 4:
        pd_action_spec.${name} = action_spec.${name};
//::         else:
	memcpy(pd_action_spec.${name}, action_spec.${name}.c_str(), ${width});
//::         #endif
//::       #endfor

//::     #endif

//::     pd_params = ["sess_hdl", "dev_id", "mbr"]
//::     if has_action_spec:
//::       pd_params += ["&pd_action_spec"]
//::     #endif
//::     pd_param_str = ", ".join(pd_params)
        return ${pd_name}(${pd_param_str});
    }

//::   #endfor

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, mbr);	
    }

//::   if not has_selector: continue
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt",
//::             "const int16_t max_grp_size"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_create_group"
//::   pd_name = pd_prefix + name
    GroupHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

	p4_pd_grp_hdl_t pd_grp_hdl;

        ${pd_name}(sess_hdl, pd_dev_tgt, max_grp_size, &pd_grp_hdl);
	return pd_grp_hdl;
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp);
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_add_member_to_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);	
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_del_member_from_group"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);	
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_deactivate_group_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);	
    }

//::   params = ["const SessionHandle_t sess_hdl",
//::             "const int8_t dev_id",
//::             "const GroupHandle_t grp",
//::             "const MemberHandle_t mbr"]
//::   param_str = ", ".join(params)
//::   name = act_prof + "_reactivate_group_member"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        return ${pd_name}(sess_hdl, dev_id, grp, mbr);	
    }

//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   if has_match_spec:
//::     params += ["const " + api_prefix + table + "_match_spec_t &match_spec"]
//::   #endif
//::   if match_type == "ternary":
//::     params += ["const int32_t priority"]
//::   #endif
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   param_str = ", ".join(params_wo)
//::   name = table + "_add_entry"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::   if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::     match_params = gen_match_params(t_info["match_fields"], field_info)
//::     for name, width in match_params:
//::       if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::       else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::       #endif
//::     #endfor

//::   #endif
        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if has_match_spec:
//::     pd_params += ["&pd_match_spec"]
//::   #endif
//::   if match_type == "ternary":
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["mbr", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//::   if not has_selector: continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   param_str = ", ".join(params_w)
//::   name = table + "_add_entry_with_selector"
//::   pd_name = pd_prefix + name
    EntryHandle_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

//::   if has_match_spec:
        ${pd_prefix}${table}_match_spec_t pd_match_spec;
//::     match_params = gen_match_params(t_info["match_fields"], field_info)
//::     for name, width in match_params:
//::       if width <= 4:
        pd_match_spec.${name} = match_spec.${name};
//::       else:
	memcpy(pd_match_spec.${name}, match_spec.${name}.c_str(), ${width});
//::       #endif
//::     #endfor

//::   #endif
        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   if has_match_spec:
//::     pd_params += ["&pd_match_spec"]
//::   #endif
//::   if match_type == "ternary":
//::     pd_params += ["priority"]
//::   #endif
//::   pd_params += ["grp", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});
        return pd_entry;
    }

//:: #endfor

//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   has_selector = action_profiles[act_prof]["selection_key"] is not None
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   params_wo = params + ["const MemberHandle_t mbr"]
//::   param_str = ", ".join(params_wo)
//::   name = table + "_set_default_entry"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["mbr", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});

        return pd_entry;
    }

//::   if not has_selector: continue
//::   params_w = params + ["const GroupHandle_t grp"]
//::   param_str = ", ".join(params_w)
//::   name = table + "_set_default_entry_with_selector"
//::   pd_name = pd_prefix + name
    int32_t ${name}(${param_str}) {
        std::cerr << "In ${name}\n";

        p4_pd_dev_target_t pd_dev_tgt;
        pd_dev_tgt.device_id = dev_tgt.dev_id;
        pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

        p4_pd_entry_hdl_t pd_entry;

//::   pd_params = ["sess_hdl", "pd_dev_tgt"]
//::   pd_params += ["grp", "&pd_entry"]
//::   pd_param_str = ", ".join(pd_params)
        ${pd_name}(${pd_param_str});

        return pd_entry;
    }
//:: #endfor

    // COUNTERS
//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "counter_read_" + counter
//::     pd_name = pd_prefix + name
    void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_flags_t &flags) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = ${pd_name}(sess_hdl, pd_dev_tgt, entry, pd_flags);
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
    }

//::     name = "counter_write_" + counter
//::     pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const EntryHandle_t entry, const ${api_prefix}counter_value_t &counter_value) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      return ${pd_name}(sess_hdl, pd_dev_tgt, entry, value);
    }

//::   else:
//::     name = "counter_read_" + counter
//::     pd_name = pd_prefix + name
    void ${name}(${api_prefix}counter_value_t &counter_value, const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_flags_t &flags) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      int pd_flags = 0;
      if(flags.read_hw_sync) pd_flags |= COUNTER_READ_HW_SYNC;

      p4_pd_counter_value_t value = ${pd_name}(sess_hdl, pd_dev_tgt, index, pd_flags);
      counter_value.packets = value.packets;
      counter_value.bytes = value.bytes;
    }

//::     name = "counter_write_" + counter
//::     pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt, const int32_t index, const ${api_prefix}counter_value_t &counter_value) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      p4_pd_counter_value_t value;
      value.packets = counter_value.packets;
      value.bytes = counter_value.bytes;

      return ${pd_name}(sess_hdl, pd_dev_tgt, index, value);
    }

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "counter_hw_sync_" + counter
//::   pd_name = pd_prefix + name
    int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      return 0;
    }

//:: #endfor

  // GLOBAL TABLE COUNTERS

//:: counter_types = ["bytes", "packets"]
//:: for type_ in counter_types:
//::   for table, t_info in table_info.items():
//::
//::     # read counter hit table
//::     name = table + "_table_read_" + type_ + "_counter_hit"
//::     pd_name = pd_prefix + name
  int64_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
  }

//::
//::     # read counter miss table
//::     name = table + "_table_read_" + type_ + "_counter_miss"
//::     pd_name = pd_prefix + name
  int64_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t &dev_tgt) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(sess_hdl, pd_dev_tgt);
  }

//::   #endfor
//:: #endfor


  // METERS

//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   params = ["const SessionHandle_t sess_hdl",
//::             "const DevTarget_t &dev_tgt"]
//::   if binding[0] == "direct":
//::     entry_or_index = "entry";
//::     params += ["const EntryHandle_t entry"]
//::   else:
//::     entry_or_index = "index";
//::     params += ["const int32_t index"]
//::   #endif
//::   if type_ == "packets":
//::     params += ["const int32_t cir_pps", "const int32_t cburst_pkts",
//::                "const int32_t pir_pps", "const int32_t pburst_pkts"]
//::   else:
//::     params += ["const int32_t cir_kbps", "const int32_t cburst_kbits",
//::                "const int32_t pir_kbps", "const int32_t pburst_kbits"]
//::   #endif
//::   param_str = ", ".join(params)
//::
//::   pd_params = ["sess_hdl", "pd_dev_tgt", entry_or_index]
//::   if type_ == "packets":
//::     pd_params += ["cir_pps", "cburst_pkts", "pir_pps", "pburst_pkts"]
//::   else:
//::     pd_params += ["cir_kbps", "cburst_kbits", "pir_kbps", "pburst_kbits"]
//::   #endif
//::   pd_param_str = ", ".join(pd_params)
//::   
//::   if binding[0] == "direct":
//::     table = binding[1]
//::     name = table + "_configure_meter_entry"
//::     pd_name = pd_prefix + name
  int32_t ${name}(${param_str}) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(${pd_param_str});
  }

//::   #endif
//::
//::   name = "meter_configure_" + meter
//::   pd_name = pd_prefix + name
  int32_t ${name}(${param_str}) {
      std::cerr << "In ${name}\n";

      p4_pd_dev_target_t pd_dev_tgt;
      pd_dev_tgt.device_id = dev_tgt.dev_id;
      pd_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

      return ${pd_name}(${pd_param_str});
  }

//:: #endfor

  // mirroring api

//:: name = "mirroring_mapping_add"
  int32_t ${name}(const int32_t mirror_id, const int32_t egress_port) {
      std::cerr << "In ${name}\n";
      return ${pd_prefix}${name}(mirror_id, egress_port);
  }

//:: name = "mirroring_mapping_delete"
  int32_t ${name}(const int32_t mirror_id) {
      std::cerr << "In ${name}\n";
      return ${pd_prefix}${name}(mirror_id);
  }

//:: name = "set_drop_tail_thr"
  int32_t ${name}(const int32_t drop_tail_size) {
      std::cerr << "In ${name}\n";
      return ::${name}(drop_tail_size);
  }

//:: name = "set_packets_per_sec"
  int32_t ${name}(const int32_t pps) {
      std::cerr << "In ${name}\n";
      return ::${name}(pps);
  }

//:: name = "mirroring_mapping_get_egress_port"
  int32_t ${name}(int32_t mirror_id) {
      std::cerr << "In ${name}\n";
      return ${pd_prefix}${name}(mirror_id);
  }

  // coalescing api

//:: name = "mirroring_set_coalescing_sessions_offset"
  int32_t ${name}(const int16_t coalescing_sessions_offset) {
      std::cerr << "In ${name}\n";
      return ${pd_prefix}${name}(coalescing_sessions_offset);
  }

//:: name = "mirroring_add_coalescing_session"
  int32_t ${name}(const int32_t mirror_id, const int32_t egress_port, const std::vector<int8_t> &header, const int16_t min_pkt_size, const int8_t timeout){
      std::cerr << "In ${name}\n";
      return ${pd_prefix}${name}(mirror_id, egress_port, &header[0], (const int8_t)header.size(), min_pkt_size, timeout);
  }

  void pg_start(const int32_t id, const std::vector<int8_t> &pg_header, const int32_t interval_ms) {
    ${pd_prefix}pg_start(id, &pg_header[0], pg_header.size(), interval_ms);
  }

  void pg_stop(const int32_t id) {
    ${pd_prefix}pg_stop(id);
  }

  void set_learning_timeout(const SessionHandle_t sess_hdl, const int8_t dev_id, const int32_t msecs) {
      ${pd_prefix}set_learning_timeout(sess_hdl, (const uint8_t)dev_id, msecs);
  }

//:: for lq in learn_quanta:
//::   rpc_msg_type = api_prefix + lq["name"] + "_digest_msg_t"
//::   rpc_entry_type = api_prefix + lq["name"] + "_digest_entry_t"
  std::map<SessionHandle_t, std::list<${rpc_msg_type}> > ${lq["name"]}_digest_queues;
  pthread_mutex_t ${lq["name"]}_mutex;

  p4_pd_status_t
  ${lq["name"]}_receive(const SessionHandle_t sess_hdl,
                        const ${rpc_msg_type} &msg) {
    pthread_mutex_lock(&${lq["name"]}_mutex);
    assert(${lq["name"]}_digest_queues.find(sess_hdl) != ${lq["name"]}_digest_queues.end());
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq["name"]}_digest_queues.find(sess_hdl);
    digest_queue->second.push_back(msg);
    pthread_mutex_unlock(&${lq["name"]}_mutex);

    return 0;
  }

  static p4_pd_status_t
  ${p4_prefix}_${lq["name"]}_cb(p4_pd_sess_hdl_t sess_hdl,
                                ${lq["msg_type"]} *msg,
                                void *cookie) {
    ${rpc_msg_type} rpc_msg;
    rpc_msg.msg_ptr = (int64_t)msg;
    rpc_msg.dev_tgt.dev_id = msg->dev_tgt.device_id;
    rpc_msg.dev_tgt.dev_pipe_id = msg->dev_tgt.dev_pipe_id;
    for (int i = 0; (msg != NULL) && (i < msg->num_entries); ++i) {
      ${rpc_entry_type} entry;
//::   for field in lq["fields"].keys():
//::     bit_width = field_info[field]["bit_width"]
//::     byte_width = (bit_width + 7 ) / 8
//::     if byte_width > 4:
      entry.${field}.insert(entry.${field}.end(), msg->entries[i].${field}, msg->entries[i].${field} + ${byte_width});
//::     else:
      entry.${field} = msg->entries[i].${field};
//::     #endif
//::   #endfor
      rpc_msg.msg.push_back(entry);
    }
    return ((${p4_prefix}Handler*)cookie)->${lq["name"]}_receive((SessionHandle_t)sess_hdl, rpc_msg);
  }

  void ${lq["name"]}_register( const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${lq["register_fn"]}(sess_hdl, dev_id, ${p4_prefix}_${lq["name"]}_cb, this);
    pthread_mutex_lock(&${lq["name"]}_mutex);
    ${lq["name"]}_digest_queues.insert(std::pair<SessionHandle_t, std::list<${rpc_msg_type}> >(sess_hdl, std::list<${rpc_msg_type}>()));
    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_deregister(const SessionHandle_t sess_hdl, const int8_t dev_id) {
    ${lq["deregister_fn"]}(sess_hdl, dev_id);
    pthread_mutex_lock(&${lq["name"]}_mutex);
    ${lq["name"]}_digest_queues.erase(sess_hdl);
    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_get_digest(${rpc_msg_type} &msg, const SessionHandle_t sess_hdl) {
    msg.msg_ptr = 0;
    msg.msg.clear();

    pthread_mutex_lock(&${lq["name"]}_mutex);
    std::map<SessionHandle_t, std::list<${rpc_msg_type}> >::iterator digest_queue = ${lq["name"]}_digest_queues.find(sess_hdl);
    if (digest_queue != ${lq["name"]}_digest_queues.end()) {
      if (digest_queue->second.size() > 0) {
        msg = digest_queue->second.front();
        digest_queue->second.pop_front();
      }
    }

    pthread_mutex_unlock(&${lq["name"]}_mutex);
  }

  void ${lq["name"]}_digest_notify_ack(const SessionHandle_t sess_hdl, const int64_t msg_ptr) {
    ${lq["notify_ack_fn"]}((p4_pd_sess_hdl_t)sess_hdl, (${lq["msg_type"]}*)msg_ptr);
  }
//:: #endfor
};
