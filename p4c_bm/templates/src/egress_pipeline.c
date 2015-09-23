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
#include <p4_sim/traffic_manager.h>
#include <p4utils/atomic_int.h>


typedef struct egress_pkt_s {
  buffered_pkt_t pkt;
  uint8_t *metadata;
  int *metadata_recirc;
  int egress;
} egress_pkt_t;

static atomic_int_t EGRESS_CB_SIZE;
static atomic_int_t USEC_INTERVAL;

static pipeline_t **egress_pipeline_instances;

void free_egress_pkt(void* e_pkt) {
  free(((egress_pkt_t *)e_pkt)->pkt.pkt_data);
  free(((egress_pkt_t *)e_pkt)->metadata);
  free(((egress_pkt_t *)e_pkt)->metadata_recirc);
  free(e_pkt);
}

int set_drop_tail_thr(const int thr) {
  if (thr <= 0) {
    return 0;
  }
  write_atomic_int(&EGRESS_CB_SIZE, thr);
  RMT_LOG(P4_LOG_LEVEL_INFO, "Set drop tail threshold to %d\n", thr);
  int i = 0;
  for (i = 0; i < NB_THREADS_PER_PIPELINE; i++) {
    cb_resize(egress_pipeline_instances[i]->cb_in, read_atomic_int(&EGRESS_CB_SIZE), free_egress_pkt);
  }
  return 0;
}

int set_packets_per_sec(const int pps) {
  if (pps <=0 ) {
    return 0;
  }
  write_atomic_int(&USEC_INTERVAL, 1000000.0 / pps);
  RMT_LOG(P4_LOG_LEVEL_INFO, "Set USEC_INTERVAL to %lu\n", read_atomic_int(&USEC_INTERVAL));
  return 0;
}

static void egress_cloning(pipeline_t *pipeline, uint8_t *pkt_data,
			   int pkt_len, uint64_t pkt_id) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_cloning\n");

  mirroring_receive(pipeline->phv,
                    metadata_recirc_digest(pipeline->phv->metadata_recirc),
                    pkt_data, pkt_len, pkt_id,
                    PKT_INSTANCE_TYPE_EGRESS_CLONE);
}

static void *processing_loop_egress(void *arg) {
  pipeline_t *pipeline = (pipeline_t *) arg;
  circular_buffer_t *cb_in = pipeline->cb_in;

#ifdef RATE_LIMITING
  struct timeval tv;
  gettimeofday(&tv, NULL);
  uint64_t next_deque = tv.tv_sec * 1000000 + tv.tv_usec + read_atomic_int(&USEC_INTERVAL);
#endif

  while(1) {
#ifdef RATE_LIMITING
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now_us = tv.tv_sec * 1000000 + tv.tv_usec;
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "next_deque %lu, now_us %lu\n", next_deque, now_us);
    if(next_deque > now_us) {
      usleep(next_deque - now_us);
    }
    next_deque += read_atomic_int(&USEC_INTERVAL);
#endif
    egress_pkt_t *e_pkt = (egress_pkt_t *) cb_read(cb_in);
    if (e_pkt == NULL) continue;
    buffered_pkt_t *b_pkt = &e_pkt->pkt;
    RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: packet dequeued\n");

    phv_clean(pipeline->phv);

    pipeline->phv->packet_id = b_pkt->pkt_id;

    parser_parse_pkt(pipeline->phv,
		     b_pkt->pkt_data, b_pkt->pkt_len,
		     pipeline->parse_state_start);
    parser_parse_metadata(pipeline->phv,
			  e_pkt->metadata, e_pkt->metadata_recirc);
    assert(!fields_get_clone_spec(pipeline->phv));

//::  if enable_intrinsic:
   /* Set dequeue metadata */
    fields_set_deq_qdepth(pipeline->phv, cb_count(cb_in));
    uint64_t enq_timestamp = fields_get_enq_timestamp(pipeline->phv);
    fields_set_deq_timedelta(pipeline->phv, get_timestamp()-enq_timestamp);
//::  #endif

    fields_set_instance_type(pipeline->phv, e_pkt->pkt.instance_type);

    free(e_pkt->metadata);
    free(e_pkt->metadata_recirc);
    if(pipeline->table_entry_fn)  /* empty egress pipeline ? */
      pipeline->table_entry_fn(pipeline->phv);

    uint8_t *pkt_data;
    int pkt_len;

    /* EGRESS MIRRORING */
    if(fields_get_clone_spec(pipeline->phv)) {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Egress mirroring\n");
      pipeline->deparse_fn(pipeline->phv, &pkt_data, &pkt_len);
      egress_cloning(pipeline, pkt_data, pkt_len, e_pkt->pkt.pkt_id);
      fields_set_clone_spec(pipeline->phv, 0);
    }

    update_checksums(pipeline->phv);
    pipeline->deparse_fn(pipeline->phv, &pkt_data, &pkt_len);

    free(b_pkt->pkt_data);

//:: if "egress_drop_ctl" in extra_metadata_name_map:
      // program uses the separate egress_drop_ctl register
      // a non-zero value means drop
    if(pipeline->phv->deparser_drop_signal ||
       metadata_get_egress_drop_ctl(metadata)) {
//:: else:
    if(pipeline->phv->deparser_drop_signal){
//:: #endif
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "dropping packet at egress\n");
      free(e_pkt);
      continue;
    }
    int egress = fields_get_egress_port(pipeline->phv);

    if(pipeline->phv->truncated_length && (pipeline->phv->truncated_length < pkt_len))
      pkt_len = pipeline->phv->truncated_length;
    
    pkt_manager_transmit(egress, pkt_data, pkt_len, b_pkt->pkt_id);
    free(e_pkt);
  }

  return NULL;
}

/* name has to be ingress or egress */
pipeline_t *pipeline_create(int id) {
  pipeline_t *pipeline = malloc(sizeof(pipeline_t));
  pipeline->name = "egress";
#ifdef RATE_LIMITING
  pipeline->cb_in = cb_init(read_atomic_int(&EGRESS_CB_SIZE), CB_WRITE_DROP, CB_READ_RETURN);
#else
  pipeline->cb_in = cb_init(read_atomic_int(&EGRESS_CB_SIZE), CB_WRITE_BLOCK, CB_READ_BLOCK);
#endif
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

//::  if enable_intrinsic:
//::    bytes = 0
//::    for header_instance in ordered_header_instances_non_virtual:
//::      h_info = header_info[header_instance]
//::      is_metadata = h_info["is_metadata"]
//::      if is_metadata: bytes += h_info["byte_width_phv"]
//::    #endfor
  uint8_t extracted_metadata[${bytes}];
  metadata_extract(extracted_metadata, metadata, NULL); /* NULL ? */
  uint32_t deflect_on_drop = metadata_get_deflect_on_drop(extracted_metadata);
  if (!deflect_on_drop) {
    int ret = cb_write(pipeline->cb_in, e_pkt);
    if (ret == 0) {
      free(e_pkt);
      RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: dropped a packet from tail\n");
    }
  } else {
    // For negative mirroring based on Qfull,
    // Set the deflection_flag in metadata to indicate that this packet 
    // is moving thru' the queuing system only to get delivered to 
    // negative mirror session.
    // Egress pipeline should act on the deflection_flag and perform 
    // explicit e2e mirroing as appropriate
    RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: deflect tail-dropped packet %p\n", metadata);
    metadata_set_deflection_flag(extracted_metadata, 1);
    metadata_set_deflect_on_drop(extracted_metadata, 0);
    metadata_set_egress_spec(extracted_metadata, 0);
    metadata_set_egress_port(extracted_metadata, 0);
    metadata_dump(metadata, extracted_metadata);
    int ret = cb_write(pipeline->cb_in, e_pkt);
    if (ret ==0) {
      free(e_pkt);
      RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: could not enqueue a deflected packet\n");
    }
  }
//::  #endif
//::  if not enable_intrinsic:
  int ret = cb_write(pipeline->cb_in, e_pkt);
  if (ret == 0) {
    free(e_pkt);
    RMT_LOG(P4_LOG_LEVEL_TRACE, "egress_pipeline: dropped a packet from tail\n");
  }
//::  #endif

  return 0;
}

int egress_pipeline_init(void) {
  egress_pipeline_instances = malloc(NB_THREADS_PER_PIPELINE * sizeof(void *));
  write_atomic_int(&USEC_INTERVAL, 0);      // asap
  //write_atomic_int(&USEC_INTERVAL, 2400); // roughly 5Mbps with 1.5KB packets
  //write_atomic_int(&USEC_INTERVAL, 1200); // roughly 10Mbps with 1.5KB packets
  write_atomic_int(&EGRESS_CB_SIZE, 15);
  int i;
  for (i = 0; i < NB_THREADS_PER_PIPELINE; i++) {
    egress_pipeline_instances[i] = pipeline_create(i);
  }

  return 0;
}
