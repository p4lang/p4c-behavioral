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

#ifndef _RMT_PRIMITIVES_H
#define _RMT_PRIMITIVES_H

#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "enums.h"
#include "fields.h"
#include "phv.h"
#include "parser.h"
#include "field_list.h"
#include "calculations.h"
#include "rmt_internal.h"
#include "metadata_recirc.h"

#include "primitives_arith.h"

static inline void ADD_HEADER(phv_data_t *phv,
			      rmt_header_instance_t header_instance) {
  if (!phv_is_valid_header(phv, header_instance)) {
    memset(
	   get_phv_ptr(phv,
		       rmt_header_instance_byte_offset_phv(header_instance)),
	   0,
	   rmt_header_instance_byte_width_phv(header_instance)
	   );
    phv_set_valid_header(phv, header_instance);
  }
}

static inline void REMOVE_HEADER(phv_data_t *phv,
				 rmt_header_instance_t header_instance) {
  phv_set_invalid_header(phv, header_instance);
}

static inline void
COPY_HEADER(phv_data_t *phv,
	    rmt_header_instance_t dst, rmt_header_instance_t src) {
  if (!phv_is_valid_header(phv, src)) {
    phv_set_invalid_header(phv, dst);
  } else {
    /* RMT_LOG(P4_LOG_LEVEL_TRACE, "copying header from %s to %s\n", */
    /* 	    RMT_HEADER_INSTANCE_STRING(src), RMT_HEADER_INSTANCE_STRING(dst)); */
    memcpy(
	   get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(dst)),
	   get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(src)),
	   rmt_header_instance_byte_width_phv(dst)
	   );
    phv_set_valid_header(phv, dst);
  }
}

static inline void
CLONE(phv_data_t *phv,
      rmt_field_list_t field_list) {
  if(!RMT_FIELD_LIST_VALID(field_list)) return;
  field_list_t *fields = field_list_get(field_list);
  int i;
  for (i = 0; i < fields->size; i++) {
    field_list_entry_t *field = &fields->entries[i];
    assert(field->type == FIELD_LIST_ENTRY_FIELD);
    phv_set_recirc_metadata_field(phv, field->data.field);
  }
}

static inline void
CLONE_INGRESS_PKT_TO_EGRESS(phv_data_t *phv,
			    rmt_field_list_t field_list) {
  CLONE(phv, field_list);
}

static inline void
CLONE_EGRESS_PKT_TO_EGRESS(phv_data_t *phv,
			   rmt_field_list_t field_list) {
  CLONE(phv, field_list);
}

static inline void
GENERATE_DIGEST(phv_data_t *phv,
		rmt_field_list_t field_list) {
  metadata_recirc_empty(phv->metadata_recirc);
  CLONE(phv, field_list);
}

static inline void
DROP(phv_data_t *phv) {
//:: if enable_pre:
  fields_set_eg_mcast_group(phv, 0);
//:: #endif
  /* behavior is different depending on the current pipeline */
  if(phv->pipeline_type == RMT_PIPELINE_INGRESS) {
    fields_set_egress_spec(phv, 511);
  }
  else {
    assert(phv->pipeline_type == RMT_PIPELINE_EGRESS);
    phv->deparser_drop_signal = 1;
  }
}

static inline void
RESUBMIT(phv_data_t *phv, rmt_field_list_t field_list) {
  phv->resubmit_signal = 1;
  if(!RMT_FIELD_LIST_VALID(field_list)) return;
  field_list_t *fields = field_list_get(field_list);
  int i;
  for (i = 0; i < fields->size; i++) {
    field_list_entry_t *field = &fields->entries[i];
    /* TODO : support headers */
    assert(field->type == FIELD_LIST_ENTRY_FIELD);
    metadata_recirc_add_field(phv->metadata_resubmit, field->data.field);
  }
}

static inline void
TRUNCATE(phv_data_t *phv, uint32_t len) {
  /* only supported in the egress pipeline */
  assert(phv->pipeline_type == RMT_PIPELINE_EGRESS);
  phv->truncated_length = len;
}

/* dst is uint32 */
/* src_len is in bytes, as in mask_len */
/* mask_len is at least 4 bytes */
static inline void
MODIFY_FIELD_UINT32(phv_data_t *phv,
		    rmt_field_instance_t dst,
		    uint8_t *src_ptr, int src_len,
		    uint8_t *mask_ptr, int mask_len) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  _MODIFY_FIELD_UINT32(dst_ptr,
		       src_ptr, src_len,
		       mask_ptr, mask_len);
  /* I don't like having to do a memory lookup here, but it's just much simpler
     this way */
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
MODIFY_FIELD_BYTE_BUF(phv_data_t *phv,
		      rmt_field_instance_t dst, int dst_len,
		      uint8_t *src_ptr, int src_len,
		      uint8_t *mask_ptr, int mask_len) {
  _MODIFY_FIELD_BYTE_BUF(get_phv_ptr(phv, rmt_field_byte_offset_phv(dst)), dst_len,
			 src_ptr, src_len,
			 mask_ptr, mask_len);
}

static inline void
MODIFY_FIELD_GENERIC(phv_data_t *phv,
		     rmt_field_instance_t dst, int dst_len,
		     uint8_t *src_ptr, int src_len,
		     uint8_t *mask_ptr, int mask_len) {
  if (dst_len == 4) {
    MODIFY_FIELD_UINT32(phv,
			dst,
			src_ptr, src_len,
			mask_ptr, mask_len);
  }
  else {
    MODIFY_FIELD_BYTE_BUF(phv,
			  dst, dst_len,
			  src_ptr, src_len,
			  mask_ptr, mask_len);
  }
}

static inline void
COPY_GENERIC(char *dst_ptr, int dst_len,
	     char *src_ptr, int src_len,
	     char *mask_ptr, int mask_len) {
  int src_idx;
  int dst_idx;
  int mask_idx;

  for (src_idx = (src_len <= dst_len) ? 0 : src_len - dst_len,
	 dst_idx = (dst_len <= src_len) ? 0 : dst_len - src_len,
	 mask_idx = mask_len - dst_len;
       src_idx < src_len;
       src_idx++, dst_idx++, mask_idx++)
    {
      dst_ptr[dst_idx] &= ~mask_ptr[dst_idx];
      dst_ptr[dst_idx] |= src_ptr[src_idx] & mask_ptr[mask_idx];
    }
}

static inline void
MODIFY_FIELD (phv_data_t *phv,
	      rmt_field_instance_t dst, int dst_len,
	      rmt_field_instance_t src, int src_len,
	      uint8_t *mask_ptr, int mask_len) {
  MODIFY_FIELD_GENERIC(phv,
		       dst, dst_len,
		       get_phv_ptr(phv, rmt_field_byte_offset_phv(src)), src_len,
		       mask_ptr, mask_len);
}

static inline void
MODIFY_FIELD_WITH_HASH_BASED_OFFSET(phv_data_t *phv,
				    rmt_field_instance_t dst, int dst_len,
				    uint8_t *base,
				    CalculationFn calculation,
				    uint8_t *size,
				    uint8_t *mask_ptr, int mask_len) {
  uint32_t computed;
  calculation(phv, (uint8_t *) &computed);
  
  uint32_t index = htonl(ntohl(*(uint32_t *) base) +
			 ntohl(computed) % ntohl(*(uint32_t *) size));
  MODIFY_FIELD_GENERIC(phv,
		       dst, dst_len,
		       (uint8_t *) &index, sizeof(uint32_t),
		       mask_ptr, mask_len);
}
				    

/* TODO Leo: temporary function with default behavior before Leo comes back and
   tests his code.*/

static inline void
ADD_UINT32(phv_data_t *phv,
           const rmt_field_instance_t dst,
           const uint8_t *src_ptr1, const int src_len1,
           const uint8_t *src_ptr2, const int src_len2) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  assert(FIELD_IS_MUTABLE(dst));
  _ADD_UINT32(dst_ptr, src_ptr1, src_len1, src_ptr2, src_len2);
  /* I don't like having to do a memory lookup here, but it's just much simpler
     this way */
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
SUBTRACT_UINT32(phv_data_t *phv,
		const rmt_field_instance_t dst,
		const uint8_t *src_ptr1, const int src_len1,
		const uint8_t *src_ptr2, const int src_len2) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  assert(FIELD_IS_MUTABLE(dst));
  _SUBTRACT_UINT32(dst_ptr, src_ptr1, src_len1, src_ptr2, src_len2);
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
SHIFT_LEFT_UINT32(phv_data_t *phv,
		  const rmt_field_instance_t dst,
		  const uint8_t *src_ptr1, const int src_len1,
		  const uint8_t *src_ptr2, const int src_len2) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  assert(FIELD_IS_MUTABLE(dst));
  _SHIFT_LEFT_UINT32(dst_ptr, src_ptr1, src_len1, src_ptr2, src_len2);
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
SHIFT_RIGHT_UINT32(phv_data_t *phv,
		   const rmt_field_instance_t dst,
		   const uint8_t *src_ptr1, const int src_len1,
		   const uint8_t *src_ptr2, const int src_len2) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  assert(FIELD_IS_MUTABLE(dst));
  _SHIFT_RIGHT_UINT32(dst_ptr, src_ptr1, src_len1, src_ptr2, src_len2);
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
SUBTRACT_FROM_FIELD_UINT32(phv_data_t *phv,
                           rmt_field_instance_t dst,
                           uint8_t *src_ptr, int src_len) {
  uint8_t *dst_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  assert(FIELD_IS_MUTABLE(dst));
  _SUBTRACT_FROM_FIELD_UINT32(dst_ptr, src_ptr, src_len);
  /* I don't like having to do a memory lookup here, but it's just much simpler
     this way */
  *(uint32_t *) dst_ptr &= rmt_field_mask(dst);
}

static inline void
ADD_GENERIC(phv_data_t *phv,
            const rmt_field_instance_t dst, const int dst_len,
            const uint8_t *src_ptr1, const int src_len1,
            const uint8_t *src_ptr2, const int src_len2) {
  if (dst_len == 4) {
    ADD_UINT32(phv,
               dst,
               src_ptr1, src_len1,
               src_ptr2, src_len2);
  }
  else {
    assert(0 && "Cannot add to non-uint32 field");
  }
}

static inline void
SUBTRACT_GENERIC(phv_data_t *phv,
		 const rmt_field_instance_t dst, const int dst_len,
		 const uint8_t *src_ptr1, const int src_len1,
		 const uint8_t *src_ptr2, const int src_len2) {
  if (dst_len == 4) {
    SUBTRACT_UINT32(phv,
		    dst,
		    src_ptr1, src_len1,
		    src_ptr2, src_len2);
  }
  else {
    assert(0 && "Cannot subtract from non-uint32 field");
  }
}

static inline void
SHIFT_LEFT_GENERIC(phv_data_t *phv,
		   const rmt_field_instance_t dst, const int dst_len,
		   const uint8_t *src_ptr1, const int src_len1,
		   const uint8_t *src_ptr2, const int src_len2) {
  if (dst_len == 4) {
    SHIFT_LEFT_UINT32(phv,
		      dst,
		      src_ptr1, src_len1,
		      src_ptr2, src_len2);
  }
  else {
    assert(0 && "Cannot subtract from non-uint32 field");
  }
}

static inline void
SHIFT_RIGHT_GENERIC(phv_data_t *phv,
		    const rmt_field_instance_t dst, const int dst_len,
		    const uint8_t *src_ptr1, const int src_len1,
		    const uint8_t *src_ptr2, const int src_len2) {
  if (dst_len == 4) {
    SHIFT_RIGHT_UINT32(phv,
		       dst,
		       src_ptr1, src_len1,
		       src_ptr2, src_len2);
  }
  else {
    assert(0 && "Cannot subtract from non-uint32 field");
  }
}

static inline void
ADD_TO_FIELD_GENERIC(phv_data_t *phv,
		     rmt_field_instance_t dst, int dst_len,
		     const uint8_t *src_ptr, int src_len) {
  const uint8_t *src_ptr2 = get_phv_ptr(phv, rmt_field_byte_offset_phv(dst));
  ADD_GENERIC(phv, dst, dst_len, src_ptr, src_len, src_ptr2, dst_len);
}

static inline void
SUBTRACT_FROM_FIELD_GENERIC(phv_data_t *phv,
                            rmt_field_instance_t dst, int dst_len,
                            uint8_t *src_ptr, int src_len) {
  if (dst_len == 4) {
    SUBTRACT_FROM_FIELD_UINT32(phv,
                               dst,
                               src_ptr, src_len);
  }
  else {
    assert(0 && "Cannot subtract to non-uint32 field");
  }
}

/* static inline void */
/* PUSH(phv_data_t *phv, */
/*      rmt_header_instance_t first_instance, */
/*      int count_to_push, int count_present) { */
/*   const uint8_t *src = */
/*     get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_instance)); */
/*   int header_byte_width = rmt_header_instance_byte_width_phv(first_instance); */
  
/*   memmove(src + header_byte_width * count_to_push, */
/* 	  src, */
/* 	  header_byte_width * count_present); */
/* } */

/* static inline void */
/* PUSH(phv_data_t *phv, */
/*      rmt_header_instance_t first_instance, */
/*      int count_to_push, int count_present) { */
/*   const uint8_t *src = */
/*     get_phv_ptr(phv, rmt_header_instance_byte_offset_phv(first_instance)); */
/*   int header_byte_width = rmt_header_instance_byte_width_phv(first_instance); */
  
/*   memmove(src + header_byte_width * count_to_push, */
/* 	  src, */
/* 	  header_byte_width * count_present); */
/* } */

static inline void
ADD(phv_data_t *phv,
    const rmt_field_instance_t dst, const int dst_len,
    const rmt_field_instance_t src1, const int src_len1,
    const rmt_field_instance_t src2, const int src_len2) {
  const uint8_t *src_ptr1 = get_phv_ptr(phv, rmt_field_byte_offset_phv(src1));
  const uint8_t *src_ptr2 = get_phv_ptr(phv, rmt_field_byte_offset_phv(src2));
  ADD_GENERIC(phv,
	      dst, dst_len,
	      src_ptr1, src_len1,
	      src_ptr2, src_len2);
}

static inline void
ADD_TO_FIELD(phv_data_t *phv,
	     rmt_field_instance_t dst, int dst_len,
	     rmt_field_instance_t src, int src_len) {
  ADD(phv, dst, dst_len, dst, dst_len, src, src_len);
}

static inline void
SUBTRACT_FROM_FIELD(phv_data_t *phv,
                    rmt_field_instance_t dst, int dst_len,
                    rmt_field_instance_t src, int src_len) {
  uint8_t *src_ptr = get_phv_ptr(phv, rmt_field_byte_offset_phv(src));
  SUBTRACT_FROM_FIELD_GENERIC(phv,
                              dst, dst_len,
                              src_ptr, src_len);
}

static inline void
BIT_XOR(phv_data_t *phv, uint8_t *dst, const uint8_t *src1, const uint8_t *src2) {
  // FIXME: Currently support only 32-bit fields.
  (*(uint32_t *)dst) = (*(uint32_t *)src1) ^ (*(uint32_t *)src2);
}

static inline void
BIT_AND(phv_data_t *phv, uint8_t *dst, const uint8_t *src1, const uint8_t *src2) {
  // FIXME: Currently support only 32-bit fields.
  (*(uint32_t *)dst) = (*(uint32_t *)src1) & (*(uint32_t *)src2);
}

static inline void
BIT_OR(phv_data_t *phv, uint8_t *dst, const uint8_t *src1, const uint8_t *src2) {
  // FIXME: Currently support only 32-bit fields.
  (*(uint32_t *)dst) = (*(uint32_t *)src1) | (*(uint32_t *)src2);
}
/* static inline void */
/* ADD_TO_FIELD_generic_uint32 (rmt_field_instance_t dst, uint32_t *src_ptr, int src_bit_width) { */
/*     // TODO: test well */

/*     // TODO: based on dst, decide values of saturate and is_signed */
/*     bool saturate = false; */
/*     bool is_signed = false; */

/*     uint32_t *dst_ptr; */
/*     uint32_t dst_mask; */
/*     uint32_t old_sign; */
/*     uint64_t temp; */

/*     dst_ptr = (uint32_t *) get_phv_ptr(rmt_field_byte_offset_phv(dst)); */
/*     dst_mask = width_mask[rmt_field_bit_width(dst)]; */

/*     if (src_bit_width > 32) { */
/*         src_ptr += src_bit_width/8 - 4; */
/*     } */

/*     if (!saturate) { */
/*         *dst_ptr = ntohl(*dst_ptr) + ntohl(*src_ptr); */
/*         *dst_ptr &= dst_mask; */
/*     } else { */
/*         temp = ntohl(*dst_ptr) + ntohl(*src_ptr); */
/*         if (!is_signed){ */
/*             if (temp & dst_mask) { */
/*                 *dst_ptr = dst_mask; */
/*             } else { */
/*                 *dst_ptr = temp; */
/*             } */
/*         } else { */
/*             if ((temp & sign_mask[rmt_field_bit_width(dst)]) != (ntohl(*dst_ptr) & sign_mask[rmt_field_bit_width(dst)])) { */
/*                 if (temp & sign_mask[rmt_field_bit_width(dst)]) { */
/*                     *dst_ptr = sign_mask[rmt_field_bit_width(dst)]; */
/*                 } else { */
/*                     *dst_ptr = ~sign_mask[rmt_field_bit_width(dst)]; */
/*                 } */
/*             } else { */
/*                 *dst_ptr = temp; */
/*             } */
/*         } */
/*     } */
/*     *dst_ptr = htons(*dst_ptr); */
/* } */

#endif
