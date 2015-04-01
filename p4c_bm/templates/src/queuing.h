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

#ifndef _RMT_QUEUING_H
#define _RMT_QUEUING_H

#include "rmt_internal.h"
#include "enums.h"

#include <p4utils/circular_buffer.h>

void queuing_init(void);
int queuing_receive(uint8_t *metadata, int *metadata_recirc,
		    void *pkt, int len, uint64_t packet_id,
		    pkt_instance_type_t instance_type);
#endif
