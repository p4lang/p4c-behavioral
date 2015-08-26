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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "enums.h"
#include "parser.h"
#include "phv.h"
#include "rmt_internal.h"
#include "value_set.h"
#include "metadata_recirc.h"

typedef void (*ExtractionFn)(phv_data_t *phv, uint8_t *hdr);

/* An extract function is defined for each header instance. */

//:: for header_instance in ordered_header_instances_non_virtual:
void _extract_${header_instance}(uint8_t *dst, uint8_t *hdr) {
  uint8_t *src;
//::   h_info = header_info[header_instance]
//::   for field in h_info["fields"]:
//::     f_info = field_info[field]
//::     d_type = f_info["data_type"]
//::     byte_offset_phv = f_info["byte_offset_phv"]
//::     byte_offset_hdr = f_info["byte_offset_hdr"]
//::     bit_offset_hdr = f_info["bit_offset_hdr"]
//::     mask = f_info["mask"]
//::     byte_width_phv = f_info["byte_width_phv"]
//::     width = f_info["bit_width"]
  src = hdr + ${byte_offset_hdr};
//::     if d_type == "uint32_t":
  EXTRACT_UINT32(dst, src, ${bit_offset_hdr}, ${width});
//::     elif d_type == "byte_buf_t":
  EXTRACT_BYTE_BUF(dst, src, ${byte_width_phv});
//::     #endif
  dst += ${byte_width_phv};
//::   #endfor
}

//:: #endfor

//:: for header_instance in ordered_header_instances_non_virtual:
static inline void extract_${header_instance}(phv_data_t *phv, uint8_t *hdr) {
//::   h_info = header_info[header_instance]
//::   byte_offset_phv = h_info["byte_offset_phv"]
  uint8_t *phv_dst = get_phv_ptr(phv, ${byte_offset_phv});
  _extract_${header_instance}(phv_dst, hdr);
  phv_set_valid_header(phv, RMT_HEADER_INSTANCE_${header_instance});
}

//:: #endfor

/* Taking care of virtual instances (next). This is trickier because the
   appropriate global variables used to keep track of virtual instances need to
   be updated and the code is different depending on where we are in the tag
   stack (thus the switch statement). */

//:: for header_instance in ordered_header_instances_virtual:
//::     h_info = header_info[header_instance]
//::     if h_info["virtual_type"] != "next": continue
//::     base_name = h_info["base_name"]
//::     first_tag = tag_stacks_first[base_name]
static void extract_${header_instance}(phv_data_t *phv, uint8_t *hdr) {
  /* hacky ? */
//::     last_name = base_name + "_LAST_"
  /* This test assumes that the instances are consecutive in the enum */
  if (phv->header_virtual_instances[RMT_HEADER_INSTANCE_${header_instance}] >=
      RMT_HEADER_INSTANCE_${first_tag} + ${base_name}_DEPTH_MAX) {
    /* TODO: too many tags extracted already */
    return;
  }
  switch (phv->header_virtual_instances[RMT_HEADER_INSTANCE_${header_instance}]) {
//::     for i in xrange(tag_stacks_depth[base_name]):
//::         tag_name = base_name + "_" + str(i) + "_"

  case RMT_HEADER_INSTANCE_${tag_name}:
    extract_${tag_name}(phv, hdr);

//::         next_name = base_name + "_next_"
//::         last_name = base_name + "_last_"
//::         for j in xrange(len(h_info["fields"])):
    phv->field_virtual_instances[RMT_FIELD_INSTANCE_${header_info[next_name]["fields"][j]}]=\
      RMT_FIELD_INSTANCE_${header_info[tag_name]["fields"][j]};
    phv->field_virtual_instances[RMT_FIELD_INSTANCE_${header_info[last_name]["fields"][j]}]=\
      RMT_FIELD_INSTANCE_${header_info[tag_name]["fields"][j]};
//::         #endfor

    break;
//:: #endfor

  default:
    /* TODO */
    break;
  }

  phv->header_virtual_instances[RMT_HEADER_INSTANCE_${last_name}] =\
    phv->header_virtual_instances[RMT_HEADER_INSTANCE_${header_instance}];

  phv->header_virtual_instances[RMT_HEADER_INSTANCE_${header_instance}] += 1;

}

//:: #endfor

/* ExtractionFn _rmt_header_instance_extraction_fn[RMT_HEADER_INSTANCE_COUNT + 1] = { */
/*   NULL, */
/* //:: for header_instance in ordered_header_instances_all: */
/*   extract_${header_instance}, */
/* //:: #endfor */
/*   NULL */
/* }; */

/* static void parse_state_end(phv_data_t *phv, uint8_t *pkt) { */
/*   phv->payload = pkt; */
/*   return; */
/* } */

//:: value_cnt = 0

//:: def get_key_byte_width(branch_on, field_info):
//::   key_width = 0
//::   for type_, switch_ref in branch_on:
//::     if type_ == "field_ref":
//::       key_width += field_info[switch_ref]["byte_width_phv"]
//::     elif type_ == "current":
//::       key_width += max(4, (switch_ref[1] + 7) / 8)
//::     #endif
//::   #endfor
//::   return key_width
//:: #enddef


/* For each parse state where we have a switch statement, we need to build a
   key. This key is built from the phv and look-aheads (current). */

//:: for state_name, p_info in parse_states.items():
//::
//::   branch_on = p_info["branch_on"]
//::   if branch_on:
static inline void build_key_${state_name}(phv_data_t *phv,
					   uint8_t *pkt, uint8_t *key) {
  /* has to be determined at runtime because of virtual instances */
  int byte_offset_phv;
  uint8_t *pkt_ptr;

  (void)byte_offset_phv;  /* Compiler reference */
  (void)pkt_ptr;  /* Compiler reference */
//::     for type_, switch_ref in branch_on:
//::       if type_ == "field_ref":
//::         field_instance = switch_ref
//::         f_info = field_info[field_instance]
  byte_offset_phv = rmt_field_byte_offset_phv(PHV_GET_FIELD(phv, RMT_FIELD_INSTANCE_${field_instance}));
//::         if f_info["data_type"] == "uint32_t":
  *(uint32_t *) key = phv_buf_field_uint32_get(phv, byte_offset_phv);
  key += sizeof(uint32_t);
//::         elif f_info["data_type"] == "byte_buf_t":
//::           byte_width = f_info["byte_width_phv"]
  memcpy(key, phv_buf_field_byte_buf_get(phv, byte_offset_phv), ${byte_width});
  key += ${byte_width};
//::         #endif
//::       elif type_ == "current":
//::         offset, width = switch_ref
//::         src_len = (width + 7) / 8
  pkt_ptr = pkt + ${offset / 8};
//::         if width <= 32:
  EXTRACT_UINT32(key, pkt_ptr, ${offset % 8}, ${width});
  key += sizeof(uint32_t);
//::         else:
  EXTRACT_BYTE_BUF(key, pkt_ptr, ${src_len});
  key += ${src_len};
//::         #endif
//::       #endif
//::     #endfor
}
//::    #endif

//::
ApplyTableFn parse_state_${state_name}(phv_data_t *phv, uint8_t *pkt) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "parsing ${state_name}\n");

  uint8_t *phv_dst;
  uint8_t *phv_src;

  (void)phv_src;  /* Compiler reference */
  (void)phv_dst;  /* Compiler reference */
//::   for call in p_info["call_sequence"]:
//::     if call[0] == "extract":
//::       header_instance_name = call[1]
  extract_${header_instance_name}(phv, pkt);
//::       h_info = header_info[header_instance_name]
//::       if h_info["virtual"]:
//::         first_tag = tag_stacks_first[h_info["base_name"]]
//::         bytes = header_info[first_tag]["bit_width"] / 8
//::       else:
//::         bytes = h_info["bit_width"] / 8
//::       #endif
  pkt += ${bytes};
  phv->payload = pkt;
//::
//::
//::     elif call[0] == "set":
//::       dst, type, value = call[1], call[2], call[3]
//::       type_dst = field_info[dst]["data_type"]
//::       dst_name = "RMT_FIELD_INSTANCE_" + dst
//::       dst_len = field_info[dst]["byte_width_phv"]
//::       dst_mask = field_info[dst]["mask"]
  phv_dst = get_phv_ptr(phv, rmt_field_byte_offset_phv(PHV_GET_FIELD(phv, ${dst_name})));
//::       if type == "latest":
//::         src = value
//::         type_src = field_info[src]["data_type"]
//::         src_name = "RMT_FIELD_INSTANCE_" + src
//::         src_len = field_info[src]["byte_width_phv"]
  phv_src = get_phv_ptr(phv, rmt_field_byte_offset_phv(PHV_GET_FIELD(phv, ${src_name})));
  SET_FIELD(phv_dst, phv_src, ${dst_len}, ${src_len}, ${dst_mask});
//::       elif type == "immediate":
//::         value_name = "value_" + dst_name
//::         src_len = len(value)
  uint8_t ${value_name}[${src_len}] = {
//::         for c in value:
    ${c},
//::         #endfor
  };
  SET_FIELD(phv_dst, ${value_name}, ${dst_len}, ${src_len}, ${dst_mask});
//::       elif type== "current":
//::         offset, width = value
//::         src_len = (width + 7) / 8
//::         value_name = "value_" + dst_name
  uint8_t ${value_name}[${src_len}];
//::         if width <= 32:
  EXTRACT_UINT32(${value_name}, pkt + {$offset / 8}, ${offset % 8}, ${width});
//::         else:
  EXTRACT_BYTE_BUF(${value_name}, pkt + {$offset / 8}, ${src_len});
//::         #endif
  SET_FIELD(phv_dst, ${value_name}, ${dst_len}, ${src_len}, ${dst_mask});
//::       #endif
//::
//::     #endif
//::
//::   #endfor
//::
//::
//::   branch_on = p_info["branch_on"]
//::   if not branch_on:
//::     assert(len(p_info["branch_to"]) == 1)
//::     case_type, value, mask, next_state = p_info["branch_to"][0]
//::     if next_state[0] == "parse_state":
  return parse_state_${next_state[1]}(phv, pkt);
//::     else:
  return tables_apply_${next_state[1]};
//::     #endif
//::   else:
//::     key_byte_width = get_key_byte_width(branch_on, field_info)
  uint8_t key[${key_byte_width}];

  build_key_${state_name}(phv, pkt, key);
//::     has_default_case = False
//::     for case_num, case in enumerate(p_info["branch_to"]):
//::       case_type, value, mask, next_state = case
//::       if case_type == "default":
//::         has_default_case = True
//::         if next_state[0] == "parse_state":
  return parse_state_${next_state[1]}(phv, pkt);
//::         else:
  return tables_apply_${next_state[1]};
//::         #endif
//::         continue
//::       #endif
//::       if case_type == "value_set":
//::         value_set = value
  if(value_set_${value_set}_contains(key)) {
//::         if next_state[0] == "parse_state":
    return parse_state_${next_state[1]}(phv, pkt);
//::         else:
    return tables_apply_${next_state[1]};
//::         #endif
  }
//::         continue
//::       #endif
//::       mask_name  = "mask_value_%d" % case_num
//::       value_name  = "case_value_%d" % case_num
//::       if case_type == "value_masked":
//::         mask_len = len(mask)
  uint8_t ${mask_name}[${mask_len}] = {
//::         for c in mask:
    ${c},
//::         #endfor
  };
//::       else:
  uint8_t *${mask_name} = NULL;
//::       #endif
//::       if case_type == "value" or case_type == "value_masked":
//::         value_len = len(value)
  uint8_t ${value_name}[${value_len}] = {
//::         for c in value:
    ${c},
//::         #endfor
  };
  
  if (cmp_values(key, ${value_name}, ${value_len}, ${mask_name})) {
//::         if next_state[0] == "parse_state":
    return parse_state_${next_state[1]}(phv, pkt);
//::         else:
    return tables_apply_${next_state[1]};
//::         #endif
  }
//::       #endif
//::     #endfor
//::     if not has_default_case:
  return NULL;
//::     #endif
//::   #endif
}

//:: #endfor

static uint8_t *parser_extract_metadata_full(phv_data_t *phv, uint8_t *pkt) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "extracting all metadata for %p\n", pkt);
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if is_metadata:
//::     bytes = h_info["bit_width"] / 8
  extract_${header_instance}(phv, pkt);
  pkt += ${bytes};
//::   #endif
//:: #endfor  
  return pkt;
}

static uint8_t *parser_extract_metadata(phv_data_t *phv, uint8_t *pkt,
					int *metadata_recirc) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "extracting metadata\n");
  if(!metadata_recirc) return parser_extract_metadata_full(phv, pkt);
  metadata_recirc_set_t *metadata_set = metadata_recirc_init(metadata_recirc);
  uint8_t *src;
  uint8_t *dst;
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if not is_metadata: continue
//::   bytes = h_info["bit_width"] / 8
//::   for field in h_info["fields"]:
//::     f_info = field_info[field]
//::     d_type = f_info["data_type"]
//::     byte_offset_phv = f_info["byte_offset_phv"]
//::     byte_offset_hdr = f_info["byte_offset_hdr"]
//::     bit_offset_hdr = f_info["bit_offset_hdr"]
//::     mask = f_info["mask"]
//::     byte_width_phv = f_info["byte_width_phv"]
//::     width = f_info["bit_width"]
  // metadata cannot be virtual, so no need to call PHV_GET_FIELD
  if(metadata_recirc_is_valid(metadata_set, RMT_FIELD_INSTANCE_${field})) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "parser_extract_metadata: extracting metadata field ${field}\n");
    src = pkt + ${byte_offset_hdr};    
    dst = get_phv_ptr(phv, ${f_info["byte_offset_phv"]});
//::     if d_type == "uint32_t":
    EXTRACT_UINT32(dst, src, ${bit_offset_hdr}, ${width});
//::     elif d_type == "byte_buf_t":
    EXTRACT_BYTE_BUF(dst, src, ${byte_width_phv});
//::     #endif
  }
//::   #endfor
  pkt += ${bytes};
//:: #endfor

  free(metadata_set);
  return pkt;
}

ApplyTableFn parser_parse_pkt(phv_data_t *phv, 
			      uint8_t *pkt, int len,
			      ParseStateFn start){
  ApplyTableFn apply_table_fn = start(phv, pkt);
  phv->payload_len = len - (phv->payload - pkt);
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "payload length: %d\n", phv->payload_len);
  return apply_table_fn;
}

void parser_parse_metadata(phv_data_t *phv,
			   uint8_t *metadata, int *metadata_recirc) {
  parser_extract_metadata(phv, metadata, metadata_recirc);
}
