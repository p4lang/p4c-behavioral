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

#include <stdint.h>

#include "checksums.h"
#include "phv.h"
#include "fields.h"
#include "primitives.h"
//#include "calculations.h"
#include "rmt_internal.h"

/* Condition functions */

//:: for idx, cond in enumerate(calculated_fields_conds):
static int check_condition_${idx + 1}(phv_data_t *phv) {
  /* this code is copied from conditional_tables.c. TODO unify and test */
//:: num_registers = len(cond)
  /*
     We are assuming that each instruction uses 1 register and they numbering
     starts with 0.
  */
  uint32_t reg[${num_registers}];

//::   for instruction in cond:
//::     dest_reg, operation, v1 = instruction[:3]
//::     if len(instruction) > 3: v2 = instruction[3]
//::     if operation == "assign_immediate":
  ${dest_reg} = (uint32_t) ${v1};
//::     elif operation == "assign_field":
  ${dest_reg} = phv_buf_field_uint32_get(phv, rmt_field_byte_offset_phv(RMT_FIELD_INSTANCE_${v1}));
  ${dest_reg} = ntohl(${dest_reg});
//::     elif operation == "not":
  ${dest_reg} = !${v1};
//::     elif operation == "or":
  ${dest_reg} = ${v1} || ${v2};
//::     elif operation == "and":
  ${dest_reg} = ${v1} && ${v2};
//::     elif operation == "==":
  ${dest_reg} = ${v1} == ${v2};
//::     elif operation == "!=":
  ${dest_reg} = ${v1} != ${v2};
//::     elif operation == "valid_header":
  ${dest_reg} = phv_is_valid_header(phv, RMT_HEADER_INSTANCE_${v1});
//::     elif operation == "valid_field":
  ${dest_reg} = phv_is_valid_field(phv, RMT_FIELD_INSTANCE_${v1});
//::     else:
//::       assert(False)
//::     #endif
//::   #endfor
  
  return reg[0];
}

//:: #endfor
/* End of condition functions */

//:: for field, verifies in calculated_fields_verify.items():
//::   f_info = field_info[field]
//::   byte_offset_phv = f_info["byte_offset_phv"]
//::   for idx, verify in enumerate(verifies):
//::     calculation, _ = verify
//::     c_info = field_list_calculations[calculation]
//::     bytes_output = c_info["output_phv_bytes"]
static int verify_${field}_${idx}(phv_data_t *phv) {
  uint8_t computed[${bytes_output}];
  calculations_${calculation}(phv, computed);
  uint8_t *field = get_phv_ptr(phv, ${byte_offset_phv});
  int result = cmp_values(computed, field, ${bytes_output}, NULL);
  return result;
}

//::     #endfor
//::   #endfor

//:: for field, updates in calculated_fields_update.items():
//::   f_info = field_info[field]
//::   byte_offset_phv = f_info["byte_offset_phv"]
//::   dst_mask = f_info["mask"]
//::   for idx, update in enumerate(updates):
//::     calculation, _ = update
//::     c_info = field_list_calculations[calculation]
//::     bytes_output = c_info["output_phv_bytes"]
static void update_${field}_${idx}(phv_data_t *phv) {
  uint8_t computed[${bytes_output}];
  calculations_${calculation}(phv, computed);
  uint8_t *field = get_phv_ptr(phv, ${byte_offset_phv});
  SET_FIELD(field, computed, ${bytes_output}, ${bytes_output}, ${dst_mask});
}

//::     #endfor
//::   #endfor

int verify_checksums(phv_data_t *phv) {
  int ret = 1;
//:: for field, verifies in calculated_fields_verify.items():
//::   for idx, verify in enumerate(verifies):
//::     _, if_cond_idx = verify
//::     if if_cond_idx:
  if(check_condition_${if_cond_idx}(phv)) {
    ret &= verify_${field}_${idx}(phv);
  }
//::     else:
  ret &= verify_${field}_${idx}(phv);
//::       break
//::     #endif

//::   #endfor
//:: #endfor
  return ret;
}

void update_checksums(phv_data_t *phv) {
//:: for field, updates in calculated_fields_update.items():
//::   for idx, update in enumerate(updates):
//::     _, if_cond_idx = update
//::     if if_cond_idx:
  if(check_condition_${if_cond_idx}(phv)) {
    update_${field}_${idx}(phv);
  }
//::     else:
  update_${field}_${idx}(phv);
//::       break
//::     #endif

//::   #endfor
//:: #endfor
}
