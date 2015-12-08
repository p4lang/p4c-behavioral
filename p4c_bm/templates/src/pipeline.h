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

#ifndef _RMT_PIPELINE_H
#define _RMT_PIPELINE_H

#include <pthread.h>
#include <sys/time.h>

#include "parser.h"
#include "deparser.h"
#include "phv.h"
#include "rmt_internal.h"
#include "enums.h"

#include <p4utils/circular_buffer.h>

#define CLONE_SPEC_FOR_DEFLECTED_TAIL_DROPS 0x0000FFFF

typedef struct pipeline_s {
  char *name;
  circular_buffer_t *cb_in;
  ParseStateFn parse_state_start;
  ApplyTableFn table_entry_fn;
  DeparseFn deparse_fn;
  pthread_t processing_thread;
  phv_data_t *phv;
} pipeline_t;

int ingress_pipeline_init(void);
int ingress_pipeline_receive(int ingress,
			     uint8_t *metadata, int *metadata_recirc,
			     void *pkt, int len, uint64_t packet_id,
			     pkt_instance_type_t instance_type);

int egress_pipeline_init(void);
int egress_pipeline_receive(int egress,
			    uint8_t *metadata, int *metadata_recirc,
			    void *pkt, int len, uint64_t packet_id,
			    pkt_instance_type_t instance_type);

void pipeline_destroy(pipeline_t *pipeline);

uint64_t get_timestamp();

int egress_pipeline_count(int egress);

#endif
