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

#ifndef _RMT_PHV_H
#define _RMT_PHV_H

#include <stdint.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <assert.h>
#include <stdbool.h>

#include "string.h"
#include "enums.h"
#include "rmt_internal.h"
#include "metadata_recirc.h"

/* Regular has to come before metadata !*/
//:: header_length = 0
//:: metadata_length = 0
//:: for header in ordered_header_instances_metadata:
//::   metadata_length += header_info[header]["byte_width_phv"]
//:: #endfor
//:: for header in ordered_header_instances_regular:
//::   header_length += header_info[header]["byte_width_phv"]
//:: #endfor
#define PHV_LENGTH ${phv_length}
#define PHV_HEADER_START 0
#define PHV_HEADER_LENGTH ${header_length}
#define PHV_METADATA_START (PHV_HEADER_START + PHV_HEADER_LENGTH)
#define PHV_METADATA_LENGTH ${metadata_length}

typedef uint8_t* byte_buf_t;

typedef struct phv_data_s {
  int id; /* ID of this PHV / pipeline */
  uint64_t packet_id; /* ID of the packet being processed */
  int pipeline_type; /* RMT_PIPELINE_INGRESS or RMT_PIPELINE_EGRESS */
  int deparser_drop_signal; /* Only valid for egress deparser, set to 1 by DROP */
  int resubmit_signal;
  metadata_recirc_set_t *metadata_resubmit; /* if not NULL, resubmit */
  uint8_t *headers;
  uint8_t *payload;  /* Everything that was not extracted by the parser */
  int payload_len;
  rmt_header_instance_t *header_virtual_instances;
  rmt_field_instance_t *field_virtual_instances;
  bool headers_valid[RMT_HEADER_INSTANCE_COUNT];
  metadata_recirc_set_t *metadata_recirc;
  int last_entry_hit;  /* for direct registers */
  uint32_t truncated_length;
} phv_data_t;

//:: num_virtual_headers = len(ordered_header_instances_virtual)
#define PHV_GET_HEADER(phv, hdr)		\
  ((hdr > ${num_virtual_headers}) ? hdr : phv->header_virtual_instances[hdr])

//:: num_virtual_fields = len(ordered_field_instances_virtual)
static inline
rmt_field_instance_t PHV_GET_FIELD(phv_data_t *phv, rmt_field_instance_t field) {
  return ((field > ${num_virtual_fields}) ? field : phv->field_virtual_instances[field]);
}

/* Extract a uint32 container. Note that the uint32 containers are also stored
   in network byte order. To perform the shift (for byte alignment), we convert
   to host byte order and then back to network byte order. There is probably
   room for improvement here. */

/* This is maybe too conservative (to please Valgrind). Given the assumptions we
   made on header alignment, we should be able to remove some of the tests, even
   if Valgrind signals an invalid read / write */
#define BYTE_ROUND_UP(x) ((x + 7) >> 3)
static inline void EXTRACT_UINT32(uint8_t *dst, uint8_t *src, int bit_offset, int width) {
  int i;
  for (i = 0; i < BYTE_ROUND_UP(width) - 1; i++) {
    dst[i] = (src[i] << bit_offset) | (src[i + 1] >> (8 - bit_offset));
  }
  dst[i] = src[i] << bit_offset;
  if((bit_offset + width) > (BYTE_ROUND_UP(width) << 3)) {
    dst[i] |= (src[i + 1] >> (8 - bit_offset));
  }
  for (i = i + 1; i < 4; i++) {
    dst[i] = 0;
  }
  *(uint32_t *) dst = htonl(ntohl(*(uint32_t *) dst) >> (32 - width));
}

/* Extract a byte_buf container. We assume these containers to be already byte
   aligned (in the incoming packet). */

static inline void *EXTRACT_BYTE_BUF(void *dst, const void *src, size_t len) {
  return memcpy(dst, src, len);
}

static inline uint8_t *get_phv_ptr(phv_data_t *phv, int offset) {
  return phv->headers + offset;
}

static inline void phv_buf_field_uint32_set(phv_data_t *phv, 
					    int byte_offset_phv,
					    uint32_t value) {
  *(uint32_t *)get_phv_ptr(phv, byte_offset_phv) = value;
}

static inline void phv_buf_field_byte_buf_set(phv_data_t *phv,
					      int byte_offset_phv,
					      byte_buf_t value,
					      size_t len) {
  memcpy(get_phv_ptr(phv, byte_offset_phv), value, len);
}

static inline uint32_t phv_buf_field_uint32_get(phv_data_t *phv,
						int byte_offset_phv) {
  return *(uint32_t *)get_phv_ptr(phv, byte_offset_phv);
}

static inline byte_buf_t phv_buf_field_byte_buf_get(phv_data_t *phv,
						    int byte_offset_phv) {
  return get_phv_ptr(phv, byte_offset_phv);
}

static inline void phv_set_valid_header(phv_data_t *phv,
					rmt_header_instance_t header_instance) {
  phv->headers_valid[header_instance] = 1;
}

static inline void phv_set_invalid_header(phv_data_t *phv,
					  rmt_header_instance_t header_instance) {
  phv->headers_valid[header_instance] = 0;
}

static inline int phv_is_valid_header(phv_data_t *phv,
				      rmt_header_instance_t header_instance){ 
  return phv->headers_valid[header_instance];
}

static inline void phv_set_recirc_metadata_field(phv_data_t *phv,
						rmt_field_instance_t field){ 
  metadata_recirc_add_field(phv->metadata_recirc, field);
}

static inline void phv_set_recirc_metadata_header(phv_data_t *phv,
						  rmt_header_instance_t header){ 
  metadata_recirc_add_header(phv->metadata_recirc, header);
}

phv_data_t *phv_init(int phv_id, int pipeline_type);

void phv_clean(phv_data_t *phv);

#endif
