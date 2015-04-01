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

#ifndef _RMT_METADATA_RECIRC_
#define _RMT_METADATA_RECIRC_

#include "enums.h"

typedef struct metadata_recirc_set_s metadata_recirc_set_t;

metadata_recirc_set_t *metadata_recirc_create(void);

void metadata_recirc_add_field(metadata_recirc_set_t *set,
			       rmt_field_instance_t field);

void metadata_recirc_remove_field(metadata_recirc_set_t *set,
			          rmt_field_instance_t field);

void metadata_recirc_add_header(metadata_recirc_set_t *set,
				rmt_header_instance_t hdr);

int *metadata_recirc_digest(metadata_recirc_set_t *set);

metadata_recirc_set_t *metadata_recirc_init(int *data);

void metadata_recirc_empty(metadata_recirc_set_t *set);

int metadata_recirc_is_valid(metadata_recirc_set_t *set,
			     rmt_field_instance_t field);

#endif
