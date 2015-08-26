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

//:: # Template file
//:: # @FIXME top matter

#ifndef _RMT_FIELDS_H
#define _RMT_FIELDS_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "enums.h"
#include "phv.h"

/* 4 different functions used to compare a key and a value. These are not called
   directly in parse state functions; instead CMP_VALUES is doing the
   demultiplexing */

static inline int switch_masked_memcmp(uint8_t *key,
				uint8_t *value,
				uint8_t *mask,
				size_t len) {
  int i;
  for (i = 0; i < len; i++) {
    if ((key[i] & mask[i]) != (value[i] & mask[i]))
      return 0;
  }
  return 1;
}

static inline int switch_memcmp(uint8_t *key,
				uint8_t *value,
				size_t len) {
  return (memcmp(key, value, len) == 0);
}

static inline int switch_masked_fast(uint32_t key,
				     uint32_t value,
				     uint32_t mask) {
  return ((key & mask) == (value & mask));
}

static inline int switch_fast(uint32_t key,
			      uint32_t value) {
  return (key == value);
}

/* Compare value1 and value2, which are 2 containers (assumed to have the same
   length) and places the result in the integer result. A mask can be
   specified. value1, value2, len and mask should be hardcoded values and only
   one of the 4 branches should remain after optimization. */

static inline int cmp_values(uint8_t *value1, uint8_t *value2, size_t len, uint8_t *mask){
  if (mask && len == 4) {
    return switch_masked_fast(*(uint32_t *) value1,
			      *(uint32_t *) value2,
			      *(uint32_t *) mask);
  }
  else if(!mask && len == 4) {
    return switch_fast(*(uint32_t *) value1,
			 *(uint32_t *) value2);
  }
  else if(mask) {
    return switch_masked_memcmp(value1, value2, mask, len);
  }
  else {
    return switch_memcmp(value1, value2, len);
  }
}

/* As of now, only used to set metadata in parse functions. Maks is assumed to
   be non-zero iff dst is a unit32 container. All the parameters should be hard
   coded values, so that the useless branches are thrown away at compile
   time. */

#define SET_FIELD(dst, src, dst_len, src_len, mask)                     \
  do {                                                                  \
    if (mask) {                                                         \
      *(uint32_t *) dst = (*(uint32_t *) (src + src_len - dst_len)) & mask; \
    }                                                                   \
    else if (src_len > dst_len)                                         \
      memcpy(dst, src + src_len - dst_len, dst_len);                    \
    else {                                                              \
      memset(dst, 0, dst_len - src_len);                                \
      memcpy(dst + (dst_len - src_len), src, src_len);                  \
    }                                                                   \
  }                                                                     \
  while(0)


//:: header_gen_objs = [				\
//::     ("bit_width", "int"),				\
//::     ("byte_offset_phv", "int"),			\
//::     ("num_fields", "int"),				\
//::     ("is_metadata", "int"),			\
//::     ("first_field", "rmt_field_instance_t"),	\
//::     ("byte_width_phv", "int")			\
//:: ]

//:: for obj_name, ret_type in header_gen_objs:

extern ${ret_type} _rmt_header_instance_${obj_name}[RMT_HEADER_INSTANCE_COUNT + 1];

static inline ${ret_type}
rmt_header_instance_${obj_name}(rmt_header_instance_t header_instance)
{
  return _rmt_header_instance_${obj_name}[header_instance];
}
//:: #endfor

/* Accessor functions for (static) field instance properties. */

//:: field_gen_objs = [                                                 \
//::     ("bit_offset_hdr", "int"),                                     \
//::     ("bit_width", "int"),                                          \
//::     ("byte_offset_hdr", "int"),                                    \
//::     ("byte_offset_phv", "int"),                                    \
//::     ("byte_width_phv", "int"),                                     \
//::     ("data_type", "rmt_field_data_type_t"),                        \
//::     ("header", "rmt_header_instance_t"),                           \
//::     ("is_metadata", "int"),					\
//::     ("mask", "int"),                                               \
//:: ]

//:: for obj_name, ret_type in field_gen_objs:

extern ${ret_type} _rmt_field_instance_${obj_name}[RMT_FIELD_INSTANCE_COUNT + 1];

static inline ${ret_type}
rmt_field_${obj_name}(rmt_field_instance_t field_instance)
{
  return _rmt_field_instance_${obj_name}[field_instance];
}
//:: #endfor

/* useful setters and getters for standard metadata fields */
//:: for name, binding in metadata_name_map.items():
//::   f_info = field_info[binding]
//::   byte_offset_phv = f_info["byte_offset_phv"]
static inline uint32_t fields_get_${name}(phv_data_t *phv) {
  return ntohl(phv_buf_field_uint32_get(phv, ${byte_offset_phv}));
}

static inline void fields_set_${name}(phv_data_t *phv, uint32_t value) {
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, htonl(value));
}

//::   #endfor


/* useful setters and getters for intrinsic metadata fields */
//:: if enable_intrinsic:
//::   for name, binding in intrinsic_metadata_name_map.items():
//::     f_info = field_info[binding]
//::     byte_offset_phv = f_info["byte_offset_phv"]
static inline uint32_t fields_get_${name}(phv_data_t *phv) {
  return ntohl(phv_buf_field_uint32_get(phv, ${byte_offset_phv}));
}

static inline void fields_set_${name}(phv_data_t *phv, uint32_t value) {
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, htonl(value));
}
//::   #endfor
//:: #endif

//:: if enable_pre:
//::   for name, binding in pre_metadata_name_map.items():
//::     f_info = field_info[binding]
//::     byte_offset_phv = f_info["byte_offset_phv"]
static inline uint32_t fields_get_${name}(phv_data_t *phv) {
  return ntohl(phv_buf_field_uint32_get(phv, ${byte_offset_phv}));
}

static inline void fields_set_${name}(phv_data_t *phv, uint32_t value) {
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, htonl(value));
}
//::   #endfor
//:: #endif

//:: for name, binding in extra_metadata_name_map.items():
//::     f_info = field_info[binding]
//::     byte_offset_phv = f_info["byte_offset_phv"]
static inline uint32_t fields_get_${name}(phv_data_t *phv) {
  return ntohl(phv_buf_field_uint32_get(phv, ${byte_offset_phv}));
}

static inline void fields_set_${name}(phv_data_t *phv, uint32_t value) {
  phv_buf_field_uint32_set(phv, ${byte_offset_phv}, htonl(value));
}
//:: #endfor


/* for debugging and testing */
void fields_print(phv_data_t *phv, rmt_field_instance_t field);
void *fields_get(phv_data_t *phv, rmt_field_instance_t field);

#endif
