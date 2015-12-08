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

#ifndef _RMT_DEPARSER_H
#define _RMT_DEPARSER_H

#include <stdint.h>

typedef int (*DeparseFn)(phv_data_t *phv, uint8_t **pkt, int *len);

int deparser_produce_pkt(phv_data_t *phv, uint8_t **pkt, int *len);

int deparser_produce_metadata(phv_data_t *phv, uint8_t **metadata);

void deparser_extract_digest(phv_data_t *phv, int *metadata_recirc, uint8_t *buffer, uint16_t *length);

// |bit_offset| is offset of the first bit in dst[0] that will be modified. The
// offset is from the MSB-side which means the first |bit_offset| MSBs in
// |dst[0]| will not be modified.
static inline void deparse_uint32(uint8_t *dst, uint8_t *src,
				  int bit_offset, int width) {
  uint32_t tmp = htonl((ntohl(*(uint32_t *) src)) << (32 - width));
  uint8_t *new_src = (uint8_t *) &tmp;
  int i;
  // First, zero-out the bits that we want to modify in |dst[0]|.
  uint8_t mask = (0xFF >> bit_offset);
  mask = ~mask;
  dst[0] = (dst[0] & mask);
  // Now, bitwise-or |8 - bit_offset| bits in |dst[0]| with |8 - bit_offset|
  // bits in |new_src[0]|.
  dst[0] |= (new_src[0] >> bit_offset);
  for (i = 1; i < BYTE_ROUND_UP(width); i++) {
    dst[i] = (new_src[i] >> bit_offset) | (new_src[i - 1] << (8 - bit_offset));
  }
  /* Added this later, to fix bug */
  /* if((bit_offset + width) > (i << 3)) */
  // FIXME: We are changing bits in |dst[i]| beyond what is specified in
  // |width|. This will work fine as long as we call |deparse_uint32| in the
  // same sequence as fields appear in a packet. But, if we deparse fields in
  // arbitrary order, we will modify bits in the following field.
  if ((bit_offset + width) > (BYTE_ROUND_UP(width) << 3))
    dst[i] = new_src[i - 1] << (8 - bit_offset);
}

static inline void deparse_byte_buf(uint8_t *dst, uint8_t *src, int len) {
  memcpy(dst, src, len);
}

//:: for header_instance in ordered_header_instances_non_virtual:
void _deparse_${header_instance}(uint8_t *src, uint8_t *hdr);
//:: #endfor

#endif
