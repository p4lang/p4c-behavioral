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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "field_list.h"
#include "deparser.h"
#include "checksums_algos.h"
#include "rmt_internal.h"

uint8_t *_buffers[NB_THREADS];

void calculations_init() {
  int i;
  for(i = 0; i < NB_THREADS; i++) {
    _buffers[i] = malloc(4096);
  }
}

/* the build_input_* functions return the length of the input */
//:: for name, c_info in field_list_calculations.items():
static int build_input_${name}(phv_data_t *phv, uint8_t *buffer) {
  uint8_t *dst = buffer;
  uint8_t *src;
//::   offset = 0
//::   # input_ is field list
//::   input_ = c_info["input"][0]
//::   field_list = field_lists[input_]
//::   for field in field_list:
//::     type_, value = field
//::     if type_ == "field_ref":
//::       f_info = field_info[value]
//::       byte_offset_phv = f_info["byte_offset_phv"]
  src = get_phv_ptr(phv, ${byte_offset_phv});
//::       d_type = f_info["data_type"]
//::       width = f_info["bit_width"]
//::       if d_type == "uint32_t":
  deparse_uint32(dst, src, ${offset}, ${width});
//::         offset += width
  dst += ${offset / 8};
//::         offset = offset % 8
//::       elif d_type == "byte_buf_t":
//::       assert(width % 8 == 0)
  assert(${offset} == 0);
  deparse_byte_buf(dst, src, ${width / 8});
  dst += ${width / 8};
//::       #endif
//::     elif type_ == "header_ref":
//::       h_info = header_info[value]
//::       byte_offset_phv = h_info["byte_offset_phv"]
//::       width = h_info["bit_width"]
  src = get_phv_ptr(phv, ${byte_offset_phv});
  _deparse_${value}(src, dst);
  dst += ${width / 8};
//::     elif type_ == "payload":
//::       prev_found = False
//::       for header_instance in ordered_header_instances_regular:
//::         if header_instance == value:
//::           prev_found = True
//::           continue
//::         #endif
//::         if not prev_found: continue
//::         h_info = header_info[header_instance]
//::         byte_offset_phv = h_info["byte_offset_phv"]
//::         width = h_info["bit_width"]
  if(phv_is_valid_header(phv, RMT_HEADER_INSTANCE_${header_instance})) {
    src = get_phv_ptr(phv, ${byte_offset_phv});
    _deparse_${header_instance}(src, dst);
    dst += ${width / 8};
  }
//::       #endfor  
  memcpy(dst, phv->payload, phv->payload_len);
  dst += phv->payload_len;
//::     elif type_ == "integer":
//::       for c in value:
  *dst++ = ${c};
//::       #endfor
//::     else:
//::       print "not supported yet"
//::       assert(False)
//::     #endif
//::   #endfor
  return (dst - buffer);
}

//:: #endfor

//:: for name, c_info in field_list_calculations.items():
void calculations_${name}(phv_data_t *phv, uint8_t *result) {
//::  input_size = c_info["input_size"]
  uint8_t *buffer = _buffers[phv->id];
  memset(buffer, 0, ${input_size});
  int len = build_input_${name}(phv, buffer);
//::   algo = c_info["algorithm"]
  ${algo}(buffer, len, result);
//::   bytes_output = c_info["output_phv_bytes"]
//::   width = c_info["output_width"]
//::   if width < 32:
//::     shift = 8 * bytes_output - width
  *(uint32_t *) result = htonl(ntohl(*(uint32_t *) result) >> ${shift});
//::   #endif
}

//:: #endfor
