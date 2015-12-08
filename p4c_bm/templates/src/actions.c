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

#include "primitives.h"
#include "enums.h"
#include "rmt_internal.h"
#include "actions.h"
#include "phv.h"
//#include "calculations.h"

#include <assert.h>
#include <stdio.h>

//:: LF_RECEIVER = 1024

// TODO: Copies for parallel semantics

//:: def format_arg (action, arg):
//::    if arg[0]=="field_ref":
//::        return "PHV_GET_FIELD(phv, RMT_FIELD_INSTANCE_%s)" % arg[1]
//::    elif arg[0]=="header_ref":
//::        return "PHV_GET_HEADER(phv, RMT_HEADER_INSTANCE_%s)" % arg[1]
//::    elif arg[0]=="param":
//::        param_start = sum(action["param_byte_widths"][0:arg[1]])
//::        return "(uint8_t *) action_data + %i" % param_start
//::    elif arg[0]=="immediate":
//::        arg_str = "(uint8_t[]) {"
//::        for byte in arg[1]:
//::            arg_str += hex(byte) + ", "
//::        #endfor
//::        arg_str = arg_str[0:-2] + "}"
//::        return arg_str
//::    elif arg[0] == "parse_state":
//::        # parse state function pointer
//::        return "parse_state_" + arg[1]
//::    elif arg[0] == "field_list":
//::        return "RMT_FIELD_LIST_" + arg[1]
//::    elif arg[0] == "p4_field_calculation":
//::        return "calculations_" + arg[1]
//::    else:
//::        return "/* WARNING: UNRECOGNIZED ARG TYPE '"+str(arg[0])+"'*/"
//::    #endif
//:: #enddef
//::
//:: def format_arg_ptr (action, arg):
//::    if arg[0]=="field_ref":
//::        return "get_phv_ptr(phv, rmt_field_byte_offset_phv(PHV_GET_FIELD(phv, RMT_FIELD_INSTANCE_%s)))" % arg[1]
//::    elif arg[0]=="header_ref":
//::        return "get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(PHV_GET_HEADER(phv, RMT_HEADER_INSTANCE_%s)))" % arg[1]
//::    elif arg[0]=="param":
//::        param_start = sum(action["param_byte_widths"][0:arg[1]])
//::        return "(uint8_t *) action_data + %i" % param_start
//::    elif arg[0]=="immediate":
//::        arg_str = "(uint8_t[]) {"
//::        for byte in arg[1]:
//::            arg_str += hex(byte) + ", "
//::        #endfor
//::        arg_str = arg_str[0:-2] + "}"
//::        return arg_str
//::    else:
//::        return "/* WARNING: UNRECOGNIZED ARG TYPE '"+str(arg[0])+"'*/"
//::    #endif
//:: #enddef

//:: def arg_width (action, arg):
//::    if arg[0]=="field_ref":
//::        return "rmt_field_bit_width("+arg[1]+")"
//::    elif arg[0]=="header_ref":
//::        return "rmt_header_bit_width("+arg[1]+")"
//::    elif arg[0]=="param":
//::        param_start = sum(action["param_byte_widths"][0:arg[1]])
//::        # TODO: It's arguable but I think we might want bit widths,
//::        #       not byte widths
//::        return str(action["param_byte_widths"][arg[1]] * 8)
//::    elif arg[0]=="immediate":
//::        return len(arg[1])*8
//::    else:
//::        return "/* WARNING: UNRECOGNIZED ARG TYPE '"+str(arg[0])+"'*/"
//::    #endif
//:: #enddef

//:: def arg_byte_width (action, arg, field_info = field_info, header_info = header_info):
//::    if arg[0]=="field_ref":
//::        return field_info[arg[1]]["byte_width_phv"]
//::    elif arg[0]=="header_ref":
//::        return header_info[arg[1]]["bit_width"] / 8
//::    elif arg[0]=="param":
//::        return action["param_byte_widths"][arg[1]]
//::    elif arg[0]=="immediate":
//::        return len(arg[1])
//::    else:
//::        assert(False)
//::    #endif
//:: #enddef

uint32_t bytes_to_uint32(uint8_t *bytes, int len) {
  return ntohl(*(uint32_t *) (bytes + len - sizeof(uint32_t)));
}

//:: for action_name, action in action_info.items():
void action_${action_name} (phv_data_t *phv, void *action_data) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "action %s\n", "${action_name}");
//::    for call in action["call_sequence"]:
//::        args = call[1]
//::        if call[0] == "MODIFY_FIELD":
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            src_type, src = args[1]
//::            src_bytes = arg_byte_width(action, args[1])
//::            if len(args) > 2:
//::              mask_str = format_arg(action, args[2])
//::              mask_bytes = arg_byte_width(action, args[2])
//::            else:
//::              mask = [255 for i in xrange(dst_bytes)]
//::              mask_str = format_arg(action, ("immediate", mask))
//::              mask_bytes = dst_bytes
//::            #endif
//::            if src_type == "field_ref":
  MODIFY_FIELD(phv,
	       ${format_arg(action, args[0])}, ${dst_bytes},
	       ${format_arg(action, args[1])}, ${src_bytes},
	       ${mask_str}, ${mask_bytes});
//::            else:
  MODIFY_FIELD_GENERIC(phv,
		       ${format_arg(action, args[0])}, ${dst_bytes},
		       ${format_arg(action, args[1])}, ${src_bytes},
		       ${mask_str}, ${mask_bytes});
//::            #endif
//::        elif call[0] == "MODIFY_FIELD_WITH_HASH_BASED_OFFSET":
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            mask = [255 for i in xrange(dst_bytes)]
//::            mask_str = format_arg(action, ("immediate", mask))
//::            mask_bytes = dst_bytes
  MODIFY_FIELD_WITH_HASH_BASED_OFFSET(phv,
				      ${format_arg(action, args[0])},
				      ${dst_bytes},
				      ${format_arg(action, args[1])},
				      ${format_arg(action, args[2])},
				      ${format_arg(action, args[3])},
				      ${mask_str}, ${mask_bytes});
//::        elif call[0] == "ADD_TO_FIELD":
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            src_type, src = args[1]
//::            src_bytes = arg_byte_width(action, args[1])
//::            assert(field_info[dst]["data_type"] == "uint32_t")
//::            if src_type == "field_ref":
  ADD_TO_FIELD(phv,
	       ${format_arg(action, args[0])}, ${dst_bytes},
	       ${format_arg(action, args[1])}, ${src_bytes});
//::            else:
  ADD_TO_FIELD_GENERIC(phv,
		       ${format_arg(action, args[0])}, ${dst_bytes},
		       ${format_arg(action, args[1])}, ${src_bytes});
//::            #endif
//::        elif call[0] in {"ADD", "SUBTRACT", "SHIFT_LEFT", "SHIFT_RIGHT"}:
  {
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            assert(field_info[dst]["data_type"] == "uint32_t")
//::            src_type1, src1 = args[1]
//::            src_bytes1 = arg_byte_width(action, args[1])
//::            if src_type1 == "field_ref":
    rmt_field_instance_t src1 = ${format_arg(action, args[1])};
    const uint8_t *src_ptr1 = get_phv_ptr(phv, rmt_field_byte_offset_phv(src1));
//::            else:
    const uint8_t *src_ptr1 = ${format_arg(action, args[1])};
//::            #endif
//::            src_type2, src2 = args[2]
//::            src_bytes2 = arg_byte_width(action, args[2])
//::            if src_type2 == "field_ref":
    rmt_field_instance_t src2 = ${format_arg(action, args[2])};
    const uint8_t *src_ptr2 = get_phv_ptr(phv, rmt_field_byte_offset_phv(src2));
//::            else:
    const uint8_t *src_ptr2 = ${format_arg(action, args[2])};
//::            #endif
    ${call[0]}_GENERIC(phv,
		       ${format_arg(action, args[0])}, ${dst_bytes},
		       src_ptr1, ${src_bytes1},
		       src_ptr2, ${src_bytes2});
  }
//::        elif call[0] == "SUBTRACT_FROM_FIELD":
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            src_type, src = args[1]
//::            src_bytes = arg_byte_width(action, args[1])
//::            assert(field_info[dst]["data_type"] == "uint32_t")
//::            if src_type == "field_ref":
  SUBTRACT_FROM_FIELD(phv,
	              ${format_arg(action, args[0])}, ${dst_bytes},
	              ${format_arg(action, args[1])}, ${src_bytes});
//::            else:
  SUBTRACT_FROM_FIELD_GENERIC(phv,
		              ${format_arg(action, args[0])}, ${dst_bytes},
		              ${format_arg(action, args[1])}, ${src_bytes});
//::            #endif
//::        elif call[0] == "GENERATE_DIGEST" and sum([x * pow(16, y) for (x, y) in zip(args[0][1], range(len(args[0][1])))]) == LF_RECEIVER:
//::            src_bytes = arg_byte_width(action, args[0])
//::            f_lf_field_list = intrinsic_metadata_name_map["lf_field_list"]
//::            f_info = field_info[f_lf_field_list]
//::            dst_bytes = f_info["byte_width_phv"]
//::            mask = [255 for i in xrange(dst_bytes)]
//::            mask_str = format_arg(action, ("immediate", mask))
//::            mask_bytes = dst_bytes
  uint32_t lf_field_list = htonl(RMT_FIELD_LIST_${args[1][1]});
  MODIFY_FIELD_GENERIC(phv,
		       ${"RMT_FIELD_INSTANCE_" + f_lf_field_list}, ${dst_bytes},
		       (uint8_t *)(&lf_field_list), sizeof(lf_field_list),
		       ${mask_str}, ${mask_bytes});
//::        elif call[0] == "CLONE_INGRESS_PKT_TO_EGRESS" or call[0] == "CLONE_EGRESS_PKT_TO_EGRESS" or call[0] == "GENERATE_DIGEST":
//::            src_bytes = arg_byte_width(action, args[0])
//::            f_clone_spec = metadata_name_map["clone_spec"]
//::            f_info = field_info[f_clone_spec]
//::            dst_bytes = f_info["byte_width_phv"]
//::            mask = [255 for i in xrange(dst_bytes)]
//::            mask_str = format_arg(action, ("immediate", mask))
//::            mask_bytes = dst_bytes
  MODIFY_FIELD_GENERIC(phv,
		       ${"RMT_FIELD_INSTANCE_" + f_clone_spec}, ${dst_bytes},
		       ${format_arg(action, args[0])}, ${src_bytes},
		       ${mask_str}, ${mask_bytes});
//::            if len(args) == 1:
//::                arg_str = "RMT_FIELD_LIST_NONE"
//::            else:
//::                arg_str = format_arg(action, args[1])
//::            #endif
  ${call[0]}(phv, ${arg_str});
//::        elif call[0] == "BIT_XOR" or call[0] == "BIT_AND" or call[0] == "BIT_OR":
//::            args_str = ""
//::            for arg in args:
//::                arg_str = format_arg(action, arg) + ", "
//::                if arg[0]=="field_ref":
//::                    arg_str = "get_phv_ptr(phv, rmt_field_byte_offset_phv(" + arg_str[0:-2] + ")), "
//::                #endif
//::                args_str += arg_str
//::            #endfor
//::            args_str = args_str[0:-2]
  ${call[0]}(phv, ${args_str});
//::        elif call[0] == "DROP":
  ${call[0]}(phv);
//::        elif call[0] == "RESUBMIT":
//::            if len(args) == 0:
//::                arg_str = "RMT_FIELD_LIST_NONE"
//::            else:
//::                arg_str = format_arg(action, args[0])
//::            #endif
  ${call[0]}(phv, ${arg_str});
//::        elif call[0] == "TRUNCATE":
//::            len_bytes = arg_byte_width(action, args[0])
  ${call[0]}(phv, bytes_to_uint32(${format_arg(action, args[0])}, ${len_bytes}));
//::        elif call[0] == "COUNT":
//::            c_name = args[0][1]
//::            c2_name = c_name
//::            c_info = counter_info[c_name]
//::            index_bytes = arg_byte_width(action, args[1])
//::            if c_info["type_"] == "packets" or c_info["type_"] == "packets_and_bytes":
//::              if c_info["type_"] == "packets_and_bytes":
//::                c2_name = c_name + "_packets"
//::              #endif
  stateful_increase_counter(&counter_${c2_name},
			    bytes_to_uint32(${format_arg_ptr(action, args[1])}, ${index_bytes}),
			    1);
//::            #endif
//::            if c_info["type_"] == "bytes" or c_info["type_"] == "packets_and_bytes":
//::              if c_info["type_"] == "packets_and_bytes":
//::                c2_name = c_name + "_bytes"
//::              #endif
  stateful_increase_counter(&counter_${c2_name},
			    bytes_to_uint32(${format_arg_ptr(action, args[1])}, ${index_bytes}),
			    fields_get_packet_length(phv));
//::            #endif
//::        elif call[0] == "EXECUTE_METER":
//::            m_name = args[0][1]
//::            m_info = meter_info[m_name]
//::            index_bytes = arg_byte_width(action, args[1])
  uint32_t ${m_name}_color;
//::            if m_info["type_"] == "packets":
  ${m_name}_color = (uint32_t) stateful_execute_meter(&meter_${m_name},
						      bytes_to_uint32(${format_arg(action, args[1])}, ${index_bytes}),
						      1);
//::            else:
  ${m_name}_color = (uint32_t) stateful_execute_meter(&meter_${m_name},
						      bytes_to_uint32(${format_arg(action, args[1])}, ${index_bytes}),
						      fields_get_packet_length(phv));
//::            #endif
//::            m_dst = args[2][1]
//::            f_info = field_info[m_dst]
//::            byte_offset_phv = f_info["byte_offset_phv"]
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, ntohl(${m_name}_color));
//::        elif call[0] == "REGISTER_WRITE":
//::            r_name = args[0][1]
//::            r_info = register_info[r_name]
//::            binding = r_info["binding"]
//::            index_bytes = arg_byte_width(action, args[1])
//::            src_type, src = args[2]
//::            idx_type, idx = args[1]
//::            arg_str = format_arg(action, args[2])
//::            src_bytes = arg_byte_width(action, args[2])
//::            if src_type == "field_ref":
//::              arg_str = "get_phv_ptr(phv, rmt_field_byte_offset_phv(%s))" % arg_str
//::            #endif
//::            if binding[0] == "direct":
//::              index = "phv->last_entry_hit"
//::            else:
//::              index = "bytes_to_uint32(%s, %d)" % (format_arg_ptr(action, args[1]), index_bytes)
//::            #endif
  stateful_write_register_${r_name}(${index}, ${arg_str}, ${src_bytes});
//::        elif call[0] == "REGISTER_READ":
//::            r_name = args[1][1]
//::            r_info = register_info[r_name]
//::            binding = r_info["binding"]
//::            index_bytes = arg_byte_width(action, args[2])
//::            dst = args[0][1]
//::            dst_bytes = field_info[dst]["byte_width_phv"]
//::            mask = [255 for i in xrange(dst_bytes)]
//::            mask_str = format_arg(action, ("immediate", mask))
//::            mask_bytes = dst_bytes
//::            if binding[0] == "direct":
//::              index = "phv->last_entry_hit"
//::            else:
//::              index = "bytes_to_uint32(%s, %d)" % (format_arg_ptr(action, args[2]), index_bytes)
//::            #endif
  stateful_read_register_${r_name}(phv,
				   ${index},
				   ${format_arg(action, args[0])}, ${dst_bytes},
				   ${mask_str}, ${mask_bytes});
//::        elif call[0] == "PUSH":
//::            first_tag = args[0][1]
//::            if len(args) > 1:
//::                count = args[1][1][-1] # only takes the last bytes
//::            else:
//::                count = 1
//::            #endif
//::            assert(count > 0)
//::            h_info = header_info[first_tag]
//::            base_name = h_info["base_name"]
//::            next_name = base_name + "_next_"
//::            last_name = base_name + "_last_"
//::            header_bytes_phv = h_info["byte_width_phv"]
//::            bytes_to_push = header_bytes_phv * count
//::            depth = tag_stacks_depth[base_name]
  rmt_header_instance_t next =
    PHV_GET_HEADER(phv, RMT_HEADER_INSTANCE_${next_name});
  rmt_header_instance_t first_tag = RMT_HEADER_INSTANCE_${first_tag};
  int nb_instances = next - first_tag;
  int count = ${count};
  if(nb_instances + ${count} > ${depth}) {
    nb_instances = ${depth - count};
    count = ${depth};
  }
  memmove(get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_tag)) + ${bytes_to_push},
	  get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_tag)),
	  nb_instances * ${header_bytes_phv});
  memset(get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_tag)),
	 0, count * ${header_bytes_phv});
  phv->header_virtual_instances[RMT_HEADER_INSTANCE_${last_name}] =
    RMT_HEADER_INSTANCE_${first_tag} + nb_instances + ${count} - 1;
  if(nb_instances + ${count} > ${depth}) {
    phv->header_virtual_instances[RMT_HEADER_INSTANCE_${next_name}] =
      first_tag + ${depth};
  }
  else {
    phv->header_virtual_instances[RMT_HEADER_INSTANCE_${next_name}] += ${count};
  }
  int instance_idx;
  for(instance_idx = 0; instance_idx < nb_instances + ${count}; instance_idx++)
    phv_set_valid_header(phv, first_tag + instance_idx);
//::        elif call[0] == "POP":
//::            first_tag = args[0][1]
//::            if len(args) > 1:
//::                count = args[1][1][-1] # only takes last byte
//::            else:
//::                count = 1
//::            #endif
//::            assert(count > 0)
//::            h_info = header_info[first_tag]
//::            base_name = h_info["base_name"]
//::            next_name = base_name + "_next_"
//::            last_name = base_name + "_last_"
//::            header_bytes_phv = h_info["byte_width_phv"]
//::            depth = tag_stacks_depth[base_name]
  rmt_header_instance_t next =
    PHV_GET_HEADER(phv, RMT_HEADER_INSTANCE_${next_name});
  rmt_header_instance_t first_tag = RMT_HEADER_INSTANCE_${first_tag};
  int nb_instances = next - first_tag;
  int count = ${count};
  if(${count} > nb_instances) {
    count = nb_instances;
  }
  memmove(get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_tag)),
	  get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_tag) + count),
	  (nb_instances - count) * ${header_bytes_phv});
  phv->header_virtual_instances[RMT_HEADER_INSTANCE_${next_name}] -= count;
  if(${count} >= nb_instances) {
    phv->header_virtual_instances[RMT_HEADER_INSTANCE_${last_name}] = 0;
  }
  else {
    phv->header_virtual_instances[RMT_HEADER_INSTANCE_${last_name}] -= count;
  }
  int instance_idx;
  for(instance_idx = nb_instances - 1; instance_idx >= nb_instances - count; instance_idx--)
    phv_set_invalid_header(phv, first_tag + instance_idx);
//::        else:
//::            arg_str = ""
//::            for arg in args:
//::                arg_str += format_arg(action, arg) + ", "
//::            #endfor
//::            arg_str = arg_str[0:-2]
  ${call[0].upper()}(phv,
		     ${arg_str});
//::        #endif    
//::    #endfor    
}

//:: #endfor

//:: for action_name, a_info in action_info.items():
static void
action_${action_name}_dump_action_data(uint8_t *data, char *action_desc, int max_length) {
  int length;
  (void)length;
//::   param_names =  a_info["param_names"]
//::   param_byte_widths = a_info["param_byte_widths"]
//::   for name, width in zip(param_names, param_byte_widths):
//::     # for unused parameters
//::     if width == 0: continue
//::     format_str = "%02x" * width
//::     args_str = ", ".join(["data[%d]" % i for i in xrange(width)])
  length = sprintf(action_desc, "%s: 0x${format_str}", "${name}", ${args_str});
  max_length -= length;
  assert(max_length > 0);
  action_desc += length;
//::     if width == 4:
//::       format_str = " ".join(["%hhu" for i in xrange(width)])
  length = sprintf(action_desc, " (${format_str})", ${args_str});
  max_length -= length;
  assert(max_length > 0);
  action_desc += length;
//::     #endif
  length = sprintf(action_desc, ",\t");
  max_length -= length;
  assert(max_length > 0);
  action_desc += length;
  data += ${width};
//::   #endfor
}

//:: #endfor

typedef void (*DumpActionDataFn)(uint8_t *data, char *action_desc, int max_length);

typedef struct action_info_s {
  ActionFn fn;
  char *name;
  DumpActionDataFn dump_data_fn;
} action_info_t;

//:: num_actions = len(action_info)
action_info_t action_names[${num_actions}] = {
  //:: for action_name, action in action_info.items():
  {action_${action_name}, "${action_name}", action_${action_name}_dump_action_data},
  //:: #endfor
};

char *action_get_name(ActionFn fn) {
  int i;
  action_info_t *action;
  for (i = 0; i < ${num_actions}; i++) {
    action = &action_names[i];
    if(fn == action->fn)
      return action->name;
  }
  return NULL; /* should not be reached */
}

void action_dump_action_data(ActionFn fn, uint8_t *data, char *entry_desc, int max_length) {
  int i;
  action_info_t *action;
  for (i = 0; i < ${num_actions}; i++) {
    action = &action_names[i];
    if(fn == action->fn)
      action->dump_data_fn(data, entry_desc, max_length);
  }
}
