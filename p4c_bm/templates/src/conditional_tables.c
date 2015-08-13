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

#include "phv.h"
#include "tables.h"
#include "fields.h"

#include <stdint.h>

//:: for conditional, ct_info in conditional_table_info.items():
/* ${ct_info["expression"]} */
void tables_apply_${conditional}(phv_data_t *phv) {
  /* First, compute the expression to branch on */
//:: num_registers = len(ct_info["expression_computation"])
  /*
     We are assuming that each instruction uses 1 register and numbering starts
     with 0.
  */
  uint32_t reg[${num_registers}];

//::   for instruction in ct_info["expression_computation"]:
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
//::     elif operation == ">":
  ${dest_reg} = ${v1} > ${v2};
//::     elif operation == ">=":
  ${dest_reg} = ${v1} >= ${v2};
//::     elif operation == "<":
  ${dest_reg} = ${v1} < ${v2};
//::     elif operation == "<=":
  ${dest_reg} = ${v1} <= ${v2};
//::     elif operation == "valid_header":
  ${dest_reg} = phv_is_valid_header(phv, RMT_HEADER_INSTANCE_${v1});
//::     elif operation == "valid_field":
  ${dest_reg} = phv_is_valid_field(phv, RMT_FIELD_INSTANCE_${v1});
//::     elif operation == "&":
  ${dest_reg} = (${v1} & ${v2});
//::     else:
//::       assert(False)
//::     #endif
//::   #endfor
  
  /* Now jump to the next table based on the result */
//::   next_true = ct_info["next_tables"][True]
//::   next_false = ct_info["next_tables"][False]
  if(reg[0]) {
//::   if next_true is not None:
    tables_apply_${next_true}(phv);
//::   #endif
    return;
  }
  else {
//::   if next_false is not None:
    tables_apply_${next_false}(phv);
//::   #endif
    return;
  }
}

//:: #endfor
