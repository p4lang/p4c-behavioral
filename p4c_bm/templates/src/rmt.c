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

#include <stdio.h>
#include <assert.h>
#include <signal.h>

#include "phv.h"
#include "parser.h"
#include "deparser.h"
#include "tables.h"
#include "action_profiles.h"
#include "enums.h"
#include "p4_sim/pd.h"
#include "p4_sim/rmt.h"
#include "rmt_internal.h"
#include "pkt_manager.h"
#include "calculations.h"

rmt_instance_t *rmt_instance;

void rmt_init(void) {
  rmt_instance = malloc(sizeof(rmt_instance_t));
  memset(rmt_instance, 0, sizeof(rmt_instance_t));

  tables_init();
  action_profiles_init();
  stateful_init();
  calculations_init();

  pkt_manager_init();
}

int rmt_process_pkt(p4_port_t ingress, void *pkt, int len) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "new packet, len : %d, ingress : %d\n",
	  len, ingress);

  return pkt_manager_receive(ingress, pkt, len);
}

void rmt_logger_set(p4_logging_f log_fn) {
  rmt_instance->logger = log_fn;
}

void rmt_log_level_set(p4_log_level_t level) {
  rmt_instance->log_level = level;
}

p4_log_level_t rmt_log_level_get() {
  return rmt_instance->log_level;
}

void rmt_transmit_register(rmt_transmit_vector_f tx_fn) {
  rmt_instance->tx_fn = tx_fn;
}
