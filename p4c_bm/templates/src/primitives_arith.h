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

#ifndef _RMT_PRIMITIVES_ARITH_H
#define _RMT_PRIMITIVES_ARITH_H

#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

static inline void
_MODIFY_FIELD_UINT32(uint8_t *dst_ptr,
		     uint8_t *src_ptr, int src_len,
		     uint8_t *mask_ptr, int mask_len) {
  uint32_t value = *(uint32_t *) (src_ptr + src_len - sizeof(uint32_t));
  uint32_t mask = *(uint32_t *) (mask_ptr + mask_len - sizeof(uint32_t));
  /* TODO: Leo, why an OR ??? */
  /* value &= mask; */
  /* *(uint32_t *) dst_ptr |= value; */
  *(uint32_t *) dst_ptr = ((*(uint32_t *) dst_ptr) & (~mask)) | (value & mask);
}

/* mask has to be at least as large as dest */
static inline void
_MODIFY_FIELD_BYTE_BUF(uint8_t *dst_ptr, int dst_len,
		       uint8_t *src_ptr, int src_len,
		       uint8_t *mask_ptr, int mask_len) {
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


/* TODO Leo: temporary function with default behavior before Leo comes back and
   tests his code.*/

static inline void
_ADD_UINT32(uint8_t *dst_ptr,
            const uint8_t *src_ptr1, const int src_len1,
            const uint8_t *src_ptr2, const int src_len2) {
  *(uint32_t *) dst_ptr = htonl(ntohl(*(const uint32_t *)
				      (src_ptr1 + src_len1 - sizeof(const uint32_t))) +
				ntohl(*(const uint32_t *)
                                      (src_ptr2 + src_len2 - sizeof(const uint32_t))));
}

static inline void
_SUBTRACT_UINT32(uint8_t *dst_ptr,
		 const uint8_t *src_ptr1, const int src_len1,
		 const uint8_t *src_ptr2, const int src_len2) {
  *(uint32_t *) dst_ptr = htonl(ntohl(*(const uint32_t *)
				      (src_ptr1 + src_len1 - sizeof(const uint32_t))) -
				ntohl(*(const uint32_t *)
                                      (src_ptr2 + src_len2 - sizeof(const uint32_t))));
}

static inline void
_SHIFT_LEFT_UINT32(uint8_t *dst_ptr,
		   const uint8_t *src_ptr1, const int src_len1,
		   const uint8_t *src_ptr2, const int src_len2) {
  *(uint32_t *) dst_ptr = htonl(ntohl(*(const uint32_t *)
				      (src_ptr1 + src_len1 - sizeof(const uint32_t))) <<
				ntohl(*(const uint32_t *)
                                      (src_ptr2 + src_len2 - sizeof(const uint32_t))));
}

static inline void
_SHIFT_RIGHT_UINT32(uint8_t *dst_ptr,
		    const uint8_t *src_ptr1, const int src_len1,
		    const uint8_t *src_ptr2, const int src_len2) {
  *(uint32_t *) dst_ptr = htonl(ntohl(*(const uint32_t *)
				      (src_ptr1 + src_len1 - sizeof(const uint32_t))) >>
				ntohl(*(const uint32_t *)
                                      (src_ptr2 + src_len2 - sizeof(const uint32_t))));
}

static inline void
_SUBTRACT_FROM_FIELD_UINT32(uint8_t *dst_ptr,
                            uint8_t *src_ptr, int src_len) {
  *(uint32_t *) dst_ptr = htonl(ntohl(*(uint32_t *) dst_ptr) - ntohl(*(uint32_t *)
				      (src_ptr + src_len - sizeof(uint32_t))));
}

#endif
