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

#include "parser.h"
#include "deparser.h"
#include "queuing.h"
#include "enums.h"
#include "pipeline.h"
#include "metadata_recirc.h"
#include "mirroring_internal.h"

static void metadata_extract_full(uint8_t *meta, uint8_t *hdr) {
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if not is_metadata: continue
  _extract_${header_instance}(meta, hdr);
  hdr += ${h_info["bit_width"] / 8};
  meta += ${h_info["byte_width_phv"]};
//:: #endfor 
}

void metadata_extract(uint8_t *meta, uint8_t *hdr,
		      int *metadata_recirc) {
  if(!metadata_recirc) return metadata_extract_full(meta, hdr);
  metadata_recirc_set_t *metadata_set = metadata_recirc_init(metadata_recirc);
  uint8_t *src;
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
  if(metadata_recirc_is_valid(metadata_set, RMT_FIELD_INSTANCE_${field})) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "metadata_extract: extracting metadata field ${field}\n");
    src = hdr + ${byte_offset_hdr};
//::     if d_type == "uint32_t":
    EXTRACT_UINT32(meta, src, ${bit_offset_hdr}, ${width});
//::     elif d_type == "byte_buf_t":
    EXTRACT_BYTE_BUF(meta, src, ${byte_width_phv});
//::     #endif
  }
  meta += ${byte_width_phv};
//::   #endfor
  // Moving |hdr| to end of ${header_instance}.
  hdr += ${bytes};
//:: #endfor

  free(metadata_set);
}

void metadata_dump(uint8_t *hdr, uint8_t *src) {
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if not is_metadata: continue
  _deparse_${header_instance}(src, hdr);
  hdr += ${h_info["bit_width"] / 8};
  src += ${h_info["byte_width_phv"]};
//:: #endfor 
}

//:: for name, binding in metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata) {
  return ntohl(*(uint32_t *) (metadata + ${metadata_offsets[binding]}));
}

void metadata_set_${name}(uint8_t *metadata, uint32_t value) {
  *(uint32_t *) (metadata + ${metadata_offsets[binding]}) = htonl(value);
}

//::   #endfor


//:: if enable_intrinsic :
//::   for name, binding in intrinsic_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata) {
  return ntohl(*(uint32_t *) (metadata + ${metadata_offsets[binding]}));
}

void metadata_set_${name}(uint8_t *metadata, uint32_t value) {
  *(uint32_t *) (metadata + ${metadata_offsets[binding]}) = htonl(value);
}
//::   #endfor
//:: #endif 

//:: if enable_pre :
//::   for name, binding in pre_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata) {
  return ntohl(*(uint32_t *) (metadata + ${metadata_offsets[binding]}));
}

void metadata_set_${name}(uint8_t *metadata, uint32_t value) {
  *(uint32_t *) (metadata + ${metadata_offsets[binding]}) = htonl(value);
}
//::   #endfor
//:: #endif

//:: for name, binding in extra_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata) {
  return ntohl(*(uint32_t *) (metadata + ${metadata_offsets[binding]}));
}

void metadata_set_${name}(uint8_t *metadata, uint32_t value) {
  *(uint32_t *) (metadata + ${metadata_offsets[binding]}) = htonl(value);
}
//:: #endfor
