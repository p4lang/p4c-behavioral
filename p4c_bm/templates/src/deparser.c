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
#include <stdint.h>

#include "enums.h"
#include "parser.h"
#include "phv.h"
#include "fields.h"
#include "rmt_internal.h"
#include "deparser.h"

//:: for header_instance in ordered_header_instances_non_virtual:
void _deparse_${header_instance}(uint8_t *src, uint8_t *hdr) {
  uint8_t *dst;
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
  // Deparsing ${field}.
  dst = hdr + ${byte_offset_hdr};
//::     if d_type == "uint32_t":
  deparse_uint32(dst, src, ${bit_offset_hdr}, ${width});
//::     elif d_type == "byte_buf_t":
  deparse_byte_buf(dst, src, ${byte_width_phv});
//::     #endif
  src += ${byte_width_phv};
//::   #endfor
}

//:: #endfor


//:: for header_instance in ordered_header_instances_non_virtual:
static inline void deparse_${header_instance}(phv_data_t *phv, uint8_t *hdr) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "deparsing ${header_instance}\n");
//::   h_info = header_info[header_instance]
//::   byte_offset_phv = h_info["byte_offset_phv"]
  uint8_t *phv_src = get_phv_ptr(phv, ${byte_offset_phv});
  _deparse_${header_instance}(phv_src, hdr);
}

//:: #endfor

/* Sum the lengths of valid headers, in order to allocate the right amount of
   memory for the outgoing packet. */

static int compute_headers_length(phv_data_t *phv) {
  int len = 0;
//:: for header_instance in ordered_header_instances_regular:
//::   h_info = header_info[header_instance]
//::   bytes = h_info["bit_width"] / 8
  if(phv_is_valid_header(phv, RMT_HEADER_INSTANCE_${header_instance})) {
    len += ${bytes};
  }
//:: #endfor  
  return len;
}

static int compute_metadata_length(phv_data_t *phv) {
//:: bytes = 0
//:: for header_instance in ordered_header_instances_metadata:
//::   h_info = header_info[header_instance]
//::   bytes += h_info["bit_width"] / 8
//:: #endfor  
  return ${bytes};
}

static uint8_t *copy_metadata(phv_data_t *phv, uint8_t *pkt) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "copying metadata\n");
//:: for header_instance in ordered_header_instances_metadata:
//::   h_info = header_info[header_instance]
//::   bytes = h_info["bit_width"] / 8
  deparse_${header_instance}(phv, pkt);
  pkt += ${bytes};
//:: #endfor  
  return pkt;
}

/* Copy the valid headers to the outgoing buffer (in the correct order). */

static uint8_t *copy_headers(phv_data_t *phv, uint8_t *pkt) {
//:: for header_instance in ordered_header_instances_regular:
//::   h_info = header_info[header_instance]
//::   bytes = h_info["bit_width"] / 8
  if(phv_is_valid_header(phv, RMT_HEADER_INSTANCE_${header_instance})) {
    deparse_${header_instance}(phv, pkt);
    pkt += ${bytes};
  }
//:: #endfor  
  return pkt;
}

/*static*/ inline void print_pkt(uint8_t *pkt, int len) {
  int i;

  /* Adding compiler reference for static data */
  for (i = 0; i < len; i++) {
    if (i > 0) printf(":");
    printf("%02X", pkt[i]);
  }
  printf("\n");
}

int deparser_produce_pkt(phv_data_t *phv,
			 uint8_t **pkt, int *len) {
  int headers_len = compute_headers_length(phv);
  *len = headers_len + phv->payload_len;
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "total length for outgoing pkt: %d\n", *len);

  *pkt = malloc(*len);

  memset(*pkt, 0, headers_len);
  copy_headers(phv, *pkt);

  /* copy payload */
  /* payload needs to be copied after headers have been set, otherwise first
     byte of payload could be corrupted. Maybe it would just be cleaner to "fix"
     the deparser for this*/
  memcpy(*pkt + headers_len, phv->payload, phv->payload_len);

  /* print_pkt(pkt, len); */

  return 0;
}

int deparser_produce_metadata(phv_data_t *phv,
			      uint8_t **metadata) {
  int len = compute_metadata_length(phv);
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "total length for outgoing meta: %d\n", len);

  *metadata = malloc(len);
  memset(*metadata, 0, len);
  copy_metadata(phv, *metadata);

  return 0;
}

void deparser_extract_digest(phv_data_t *phv, int *metadata_recirc, uint8_t *buffer, uint16_t *length) {
    metadata_recirc_set_t *metadata_set = metadata_recirc_init(metadata_recirc);
    uint8_t *src, *dst;
    *length = 0;
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
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
      RMT_LOG(P4_LOG_LEVEL_TRACE, "Extracting recirc-metadata field ${field}\n");
      src = get_phv_ptr(phv, ${f_info["byte_offset_phv"]});
      dst = buffer + (*length);
//::     if d_type == "uint32_t":
      deparse_uint32(dst, src, 0, ${width});
//::       width = ((width + 7 ) >> 3)
      (*length) += ${width};
//::     elif d_type == "byte_buf_t":
      deparse_byte_buf(dst, src, ${byte_width_phv});
      (*length) += ${byte_width_phv};
//::     #endif
    }
//::   #endfor
//:: #endfor

    free(metadata_set);
}
