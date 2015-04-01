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

#ifndef _RMT_RMT_INTERNAL_H
#define _RMT_RMT_INTERNAL_H

#include <stdint.h>
#include <stdbool.h>

#include "p4_sim/rmt.h"
#include "enums.h"

#define NB_THREADS_PER_PIPELINE 8
#define NB_THREADS (2 * NB_THREADS_PER_PIPELINE)

#ifdef DEBUG
#define RMT_LOG(level, ...)						\
  do {									\
    if(rmt_instance->logger && level <= rmt_instance->log_level) {	\
      rmt_instance->logger(__VA_ARGS__);				\
    }									\
  }									\
  while(0)
#else
#define RMT_LOG(level, ...) ;
#endif

#define RMT_PRINT(...)							\
  do {									\
    rmt_instance->logger(__VA_ARGS__);					\
  }									\
  while(0)


#define RMT_LOG_LEVEL (rmt_instance->log_level)

typedef struct rmt_instance_s {
  p4_logging_f logger;
  p4_log_level_t log_level;
  rmt_transmit_vector_f tx_fn;
} rmt_instance_t;

extern rmt_instance_t *rmt_instance;

/* TODO: define a different format for each queue (cb) ? */

typedef struct buffered_pkt_s {
  pkt_instance_type_t instance_type;
  uint8_t *pkt_data;
  int pkt_len;
  uint64_t pkt_id;
} buffered_pkt_t;

#endif
