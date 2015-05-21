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
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#include "pipeline.h"
#include "parser.h"
#include "metadata_utils.h"
#include "phv.h"
#include "rmt_internal.h"
#include "fields.h"
#include "deparser.h"
#include "queuing.h"
#include "pkt_manager.h"
#include "metadata_recirc.h"
#include "checksums.h"
#include "tables.h"
#include "mirroring_internal.h"
#include <p4utils/atomic_int.h>


typedef struct egress_pkt_s {
  buffered_pkt_t pkt;
  uint8_t *metadata;
  int *metadata_recirc;
  int egress;
} egress_pkt_t;

#define EGRESS_CB_SIZE 1024

static pipeline_t **egress_pipeline_instances;

static void egress_cloning(pipeline_t *pipeline, egress_pkt_t *e_pkt) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_cloning\n");

  mirroring_receive(pipeline->phv,
                    metadata_recirc_digest(pipeline->phv->metadata_recirc),
                    e_pkt->pkt.pkt_data, e_pkt->pkt.pkt_len, e_pkt->pkt.pkt_id,
                    PKT_INSTANCE_TYPE_EGRESS_CLONE);
}

static void *processing_loop_egress(void *arg) {
  pipeline_t *pipeline = (pipeline_t *) arg;
  circular_buffer_t *cb_in = pipeline->cb_in;

  while(1) {
    egress_pkt_t *e_pkt = (egress_pkt_t *) cb_read(cb_in);
    if (e_pkt == NULL) continue;
    buffered_pkt_t *b_pkt = &e_pkt->pkt;
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: packet dequeued\n");

    phv_clean(pipeline->phv);

    pipeline->phv->packet_id = b_pkt->pkt_id;

    parser_parse_pkt(pipeline->phv,
		     b_pkt->pkt_data, b_pkt->pkt_len,
		     pipeline->parse_state_start);
    parser_parse_metadata(pipeline->phv,
			  e_pkt->metadata, e_pkt->metadata_recirc);
    assert(!fields_get_clone_spec(pipeline->phv));

    fields_set_instance_type(pipeline->phv, e_pkt->pkt.instance_type);

    free(e_pkt->metadata);
    free(e_pkt->metadata_recirc);
    if(pipeline->table_entry_fn)  /* empty egress pipeline ? */
      pipeline->table_entry_fn(pipeline->phv);

    /* EGRESS MIRRORING */
    int do_cloning = 0;
    if(fields_get_clone_spec(pipeline->phv)) {
      do_cloning = 1;
      egress_cloning(pipeline, e_pkt);
      fields_set_clone_spec(pipeline->phv, 0);
    }

    /* TODO: move to deparser module (checksums and signal drop)? */
    if(pipeline->phv->deparser_drop_signal){
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "dropping packet at egress\n");
      free(e_pkt);
      continue;
    }
    update_checksums(pipeline->phv);
    uint8_t *pkt_data;
    int pkt_len;
    pipeline->deparse_fn(pipeline->phv, &pkt_data, &pkt_len);
    if(!do_cloning) {
      free(b_pkt->pkt_data);
    }
    int egress = fields_get_egress_port(pipeline->phv);
    int ingress = fields_get_ingress_port(pipeline->phv);

    if(pipeline->phv->truncated_length && (pipeline->phv->truncated_length < pkt_len))
      pkt_len = pipeline->phv->truncated_length;
    
    pkt_manager_transmit(egress, pkt_data, pkt_len, b_pkt->pkt_id, ingress);
    free(e_pkt);
  }

  return NULL;
}

/* name has to be ingress or egress */
pipeline_t *pipeline_create(int id) {
  pipeline_t *pipeline = malloc(sizeof(pipeline_t));
  pipeline->name = "egress";
  pipeline->cb_in = cb_init(EGRESS_CB_SIZE, CB_WRITE_BLOCK, CB_READ_BLOCK);
  pipeline->parse_state_start = parse_state_start;
//:: if egress_entry_table is not None:
  pipeline->table_entry_fn = tables_apply_${egress_entry_table};
//:: else:
  pipeline->table_entry_fn = NULL;
//:: #endif
  pipeline->deparse_fn = deparser_produce_pkt;
  pipeline->phv = phv_init(NB_THREADS_PER_PIPELINE + id, RMT_PIPELINE_EGRESS);

  /* packet processing loop */
  pthread_create(&pipeline->processing_thread, NULL,
		 processing_loop_egress, (void *) pipeline);

  return pipeline;
}

int egress_pipeline_count(int egress) {
  pipeline_t *pipeline = egress_pipeline_instances[egress %
						   NB_THREADS_PER_PIPELINE];
  return cb_count(pipeline->cb_in);
}

int egress_pipeline_receive(int egress,
			    uint8_t *metadata,
			    int *metadata_recirc,
			    void *pkt, int len, uint64_t packet_id,
			    pkt_instance_type_t instance_type) {
  pipeline_t *pipeline = egress_pipeline_instances[egress %
						   NB_THREADS_PER_PIPELINE];
  
  egress_pkt_t *e_pkt = malloc(sizeof(egress_pkt_t));
  buffered_pkt_t *b_pkt = &e_pkt->pkt;
  e_pkt->egress = egress;
  e_pkt->metadata = metadata;
  e_pkt->metadata_recirc = metadata_recirc;
  b_pkt->instance_type = instance_type;
  b_pkt->pkt_data = pkt;
  b_pkt->pkt_len = len;
  b_pkt->pkt_id = packet_id;

  cb_write(pipeline->cb_in, e_pkt);

  return 0;
}

int egress_pipeline_init(void) {
  egress_pipeline_instances = malloc(NB_THREADS_PER_PIPELINE * sizeof(void *));
  int i;
  for (i = 0; i < NB_THREADS_PER_PIPELINE; i++) {
    egress_pipeline_instances[i] = pipeline_create(i);
  }

  return 0;
}
