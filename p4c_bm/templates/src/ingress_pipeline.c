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
#include <pthread.h>
#include <sys/time.h>

#include "pipeline.h"
#include "parser.h"
#include "phv.h"
#include "lf.h"
#include "rmt_internal.h"
#include "fields.h"
#include "deparser.h"
#include "queuing.h"
#include "metadata_recirc.h"
#include "metadata_utils.h"
#include "checksums.h"

#define INGRESS_CB_SIZE 1024

typedef struct ingress_pkt_s {
  buffered_pkt_t pkt;
  uint8_t *metadata;
  int *metadata_recirc;
  int ingress;
} ingress_pkt_t;

pipeline_t **ingress_pipeline_instances;

/* i_pkt is the original packet */
static void ingress_cloning(pipeline_t *pipeline, ingress_pkt_t *i_pkt) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "ingress_cloning\n");

  uint8_t *metadata;
  deparser_produce_metadata(pipeline->phv, &metadata);
//:: if enable_pre:
  metadata_set_eg_mcast_group(metadata, 0);
//:: #endif
  queuing_receive(metadata,
		  metadata_recirc_digest(pipeline->phv->metadata_recirc),
  		  i_pkt->pkt.pkt_data, i_pkt->pkt.pkt_len, i_pkt->pkt.pkt_id,
		  PKT_INSTANCE_TYPE_INGRESS_CLONE);
}

static inline void set_standard_metadata(pipeline_t *pipeline,
					 ingress_pkt_t *i_pkt) {
  fields_set_ingress_port(pipeline->phv, i_pkt->ingress);
  fields_set_packet_length(pipeline->phv, i_pkt->pkt.pkt_len);
  fields_set_instance_type(pipeline->phv, i_pkt->pkt.instance_type);
}

static void *processing_loop_ingress(void *arg) {
  pipeline_t *pipeline = (pipeline_t *) arg;
  circular_buffer_t *cb_in = pipeline->cb_in;
  
  while(1) {
    ingress_pkt_t *i_pkt = (ingress_pkt_t *) cb_read(cb_in);
    buffered_pkt_t *b_pkt = &i_pkt->pkt;
    RMT_LOG(P4_LOG_LEVEL_TRACE, "ingress_pipeline: packet dequeued\n");

    phv_clean(pipeline->phv);

    pipeline->phv->packet_id = b_pkt->pkt_id;

    /* set standard metadata */
    set_standard_metadata(pipeline, i_pkt);


//::  if enable_intrinsic:
  /* Set ingress timestamp */
    fields_set_ingress_global_timestamp(pipeline->phv, get_timestamp());
//::  #endif 

    ApplyTableFn table_entry_fn = parser_parse_pkt(pipeline->phv,
						   b_pkt->pkt_data, b_pkt->pkt_len,
						   pipeline->parse_state_start);

    if(i_pkt->metadata && i_pkt->metadata_recirc) {
      parser_parse_metadata(pipeline->phv, i_pkt->metadata, i_pkt->metadata_recirc);
    }

    /* verify checksums */
    int checksums_correct = verify_checksums(pipeline->phv);
    if(!checksums_correct) {
      RMT_LOG(P4_LOG_LEVEL_INFO,
	      "one checksum is incorrect\n");
    }
    else {
      RMT_LOG(P4_LOG_LEVEL_TRACE,
	      "all checksums are correct\n");
    }

    if(table_entry_fn)
      table_entry_fn(pipeline->phv);

    /* INGRESS MIRRORING */
    int do_cloning = 0;
    if(fields_get_clone_spec(pipeline->phv)) {
      do_cloning = 1;
      ingress_cloning(pipeline, i_pkt);
      fields_set_clone_spec(pipeline->phv, 0);
    }

//:: if len(learn_quanta) > 0 and enable_intrinsic:
    const rmt_field_list_t lf_field_list = fields_get_lf_field_list(pipeline->phv);
    lf_receive(pipeline->phv, lf_field_list);
//:: #endif

    /* SEND PACKET TO EGRESS */
    uint8_t *metadata;
    uint8_t *pkt_data;
    int pkt_len;

//:: if "copy_to_cpu" in extra_metadata_name_map:
#define CPU_PORT 64 // TODO: improve
    // program supports copy to cpu
    if(fields_get_copy_to_cpu(pipeline->phv)) {
      pipeline->deparse_fn(pipeline->phv, &pkt_data, &pkt_len);
      deparser_produce_metadata(pipeline->phv, &metadata);
//::   if enable_pre:
      metadata_set_eg_mcast_group(metadata, 0);
//::   #endif
      metadata_set_egress_spec(metadata, CPU_PORT);
      queuing_receive(metadata,
		      metadata_recirc_digest(pipeline->phv->metadata_recirc),
		      pkt_data, pkt_len, i_pkt->pkt.pkt_id,
		      PKT_INSTANCE_TYPE_INGRESS_CLONE);
      fields_set_copy_to_cpu(pipeline->phv, 0);
    }
//:: #endif


    pipeline->deparse_fn(pipeline->phv, &pkt_data, &pkt_len);
    deparser_produce_metadata(pipeline->phv, &metadata);

    if(pipeline->phv->resubmit_signal) {
      RMT_LOG(P4_LOG_LEVEL_TRACE, "resubmitting packet\n");
      /* TODO: this is dirty, we are producing the metadata twice. This is just
	 for the sake of testing resubmit. */
      uint8_t *metadata_resubmit;
      deparser_produce_metadata(pipeline->phv, &metadata_resubmit);
      uint8_t *pkt_resubmit = malloc(pkt_len);
      memcpy(pkt_resubmit, pkt_data, pkt_len);
      ingress_pipeline_receive(i_pkt->ingress,
			       metadata_resubmit,
			       metadata_recirc_digest(pipeline->phv->metadata_resubmit),
			       pkt_resubmit, pkt_len, pipeline->phv->packet_id,
			       PKT_INSTANCE_TYPE_RESUBMIT);
    }


    if(!do_cloning) {
      /* we cannot free data before because payload is use by deparser */
      /* TODO: we don't actually need that payload before end of egress pipeline
       */ 
      free(b_pkt->pkt_data);
    }
    queuing_receive(metadata, NULL,
    		    pkt_data, pkt_len, b_pkt->pkt_id,
		    b_pkt->instance_type);

    free(i_pkt);
  }

  return NULL;
}

static pipeline_t *pipeline_create(int id) {
  pipeline_t *pipeline = malloc(sizeof(pipeline_t));
  pipeline->name = "ingress";
  pipeline->cb_in = cb_init(INGRESS_CB_SIZE, CB_WRITE_BLOCK, CB_READ_BLOCK);
  pipeline->parse_state_start = parse_state_start;
  pipeline->table_entry_fn = NULL;
  pipeline->deparse_fn = deparser_produce_pkt;
  pipeline->phv = phv_init(id, RMT_PIPELINE_INGRESS);

  pthread_create(&pipeline->processing_thread, NULL,
		 processing_loop_ingress, (void *) pipeline);

  return pipeline;
}

int ingress_pipeline_receive(int ingress,
			     uint8_t *metadata,
			     int *metadata_recirc,
			     void *pkt, int len, uint64_t packet_id,
			     pkt_instance_type_t instance_type) {
  pipeline_t *pipeline = ingress_pipeline_instances[ingress %
						    NB_THREADS_PER_PIPELINE];
  
  ingress_pkt_t *i_pkt = malloc(sizeof(ingress_pkt_t));
  buffered_pkt_t *b_pkt = &i_pkt->pkt;
  i_pkt->ingress = ingress;
  i_pkt->metadata = metadata;
  i_pkt->metadata_recirc = metadata_recirc;
  b_pkt->instance_type = instance_type;
  b_pkt->pkt_data = pkt;
  b_pkt->pkt_len = len;
  b_pkt->pkt_id = packet_id;
  cb_write(pipeline->cb_in, i_pkt);
  return 0;
}

int ingress_pipeline_init(void) {
  ingress_pipeline_instances = malloc(NB_THREADS_PER_PIPELINE * sizeof(void *)); 
  int i;
  for (i = 0; i < NB_THREADS_PER_PIPELINE; i++) {
    ingress_pipeline_instances[i] = pipeline_create(i);
  }

  return 0;
}
