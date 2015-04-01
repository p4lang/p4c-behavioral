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

#ifndef _RMT_PARSER_H
#define _RMT_PARSER_H

#include <stdint.h>

#include "phv.h"
#include "tables.h"

#define BYTE_ROUND_UP(x) ((x + 7) >> 3)

typedef ApplyTableFn (*ParseStateFn)(phv_data_t *phv, uint8_t *pkt);

ApplyTableFn parser_parse_pkt(phv_data_t *phv,
			      uint8_t *pkt, int len,
			      ParseStateFn start);

void parser_parse_metadata(phv_data_t *phv,
			   uint8_t *metadata, int *metadata_recirc);

/* parse states */

/* should include parse_state_start */
//:: for state_name, p_info in parse_states.items():
ApplyTableFn parse_state_${state_name}(phv_data_t *phv, uint8_t *pkt);
//:: #endfor


//:: for header_instance in ordered_header_instances_non_virtual:
void _extract_${header_instance}(uint8_t *dst, uint8_t *hdr);
//:: #endfor


#endif
