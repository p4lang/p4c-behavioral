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

#include <pthread.h>

#include "queuing.h"
#include "metadata_utils.h"
#include "parser.h"
#include "deparser.h"
#include "enums.h"
#include "pipeline.h"
#include "metadata_recirc.h"
#include "mirroring_internal.h"
#include "pre_internal.h"

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"

typedef struct queuing_pkt_s {
  buffered_pkt_t pkt;
  uint8_t *metadata;
  int *metadata_recirc;
} queuing_pkt_t;

#define QUEUING_CB_SIZE 1024

circular_buffer_t *rmt_queuing_cb; /* only 1 */

pthread_t queuing_thread;

uint8_t *metadata;

//:: bytes = 0
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if is_metadata: bytes += h_info["byte_width_phv"]
//:: #endfor

//:: if enable_pre:
static int pre_replication(queuing_pkt_t *q_pkt, uint8_t *metadata)
{
    int replication = 0;
    buffered_pkt_t *b_pkt = &q_pkt->pkt;

    uint16_t eg_mcast_group = metadata_get_eg_mcast_group(metadata);
    if (eg_mcast_group) {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "replicating packets\n");
      mc_process_and_replicate(eg_mcast_group, metadata, q_pkt->metadata_recirc, b_pkt);
      // Free the packet once it is replicated
      free(b_pkt->pkt_data);
      free(q_pkt->metadata);
      free(q_pkt->metadata_recirc);
      free(q_pkt);
      replication = 1;
    }
    return replication;
}
//:: #endif

static void *pkt_processing_loop(void *arg) {
  while(1) {
    queuing_pkt_t *q_pkt = (queuing_pkt_t *) cb_read(rmt_queuing_cb);
    buffered_pkt_t *b_pkt = &q_pkt->pkt;
    RMT_LOG(P4_LOG_LEVEL_TRACE, "queuing system: packet dequeued\n");
    memset(metadata, 0, ${bytes});
    metadata_extract(metadata, q_pkt->metadata, q_pkt->metadata_recirc);

    /* after this point, no one should use q_pkt->metadata */

    int mirror_id = metadata_get_clone_spec(metadata);
    metadata_set_clone_spec(metadata, 0);
    int egress_port;
//:: if enable_pre:
    if (pre_replication(q_pkt, metadata)) {
      continue;
    }
//:: #endif
    if(mirror_id == 0) {
      egress_port = metadata_get_egress_spec(metadata);
      /* TODO: formalize somewhere that 511 is the drop port for this target */
//:: if "ingress_drop_ctl" in extra_metadata_name_map:
      // program uses the separate ingress_drop_ctl register
      // a non-zero value means drop
      if(egress_port = 511 || metadata_get_ingress_drop_ctl(metadata)) {
//:: else:
      if(egress_port == 511) {
//:: #endif
	RMT_LOG(P4_LOG_LEVEL_VERBOSE, "dropping packet at ingress\n");
	free(b_pkt->pkt_data);
	free(q_pkt->metadata);
	free(q_pkt->metadata_recirc);
	free(q_pkt);
	continue;
      }
    }
    else {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "mirror id is %d\n", mirror_id);
      egress_port = ${pd_prefix}mirroring_mapping_get_egress_port(mirror_id);
      if (egress_port < 0) {
	RMT_LOG(P4_LOG_LEVEL_WARN,
		"no mapping for mirror id %d, dropping packet\n", mirror_id);
	free(b_pkt->pkt_data);
	free(q_pkt->metadata);
	free(q_pkt->metadata_recirc);
	free(q_pkt);
	continue;
      }
      RMT_LOG(P4_LOG_LEVEL_TRACE,
             "queuing system: cloned packet, mirror_id -> egress_port : %d -> %d\n",
             mirror_id, egress_port);
    }
    metadata_set_egress_port(metadata, egress_port);
    RMT_LOG(P4_LOG_LEVEL_TRACE, "egress port set to %d\n", egress_port);

    pkt_instance_type_t instance_type = b_pkt->instance_type;
    metadata_set_instance_type(metadata, instance_type);
    RMT_LOG(P4_LOG_LEVEL_TRACE, "instance type set to %d\n", instance_type);

//::  if enable_intrinsic:
    /* Set enqueue metadata */
    metadata_set_enq_qdepth(metadata, egress_pipeline_count(egress_port));
    metadata_set_enq_timestamp(metadata, get_timestamp());
//::  #endif 
    
    metadata_dump(q_pkt->metadata, metadata);
    egress_pipeline_receive(egress_port,
			    q_pkt->metadata, q_pkt->metadata_recirc,
			    b_pkt->pkt_data, b_pkt->pkt_len, b_pkt->pkt_id,
			    b_pkt->instance_type);
    free(q_pkt);
  }

  return NULL;
}

void queuing_init(void) {
  rmt_queuing_cb = cb_init(QUEUING_CB_SIZE, CB_WRITE_BLOCK, CB_READ_BLOCK);

  metadata = malloc(${bytes});

//:: if enable_pre:
    mc_pre_init();
//:: #endif
  pthread_create(&queuing_thread, NULL,
		 pkt_processing_loop, NULL);
}

int queuing_receive(uint8_t *metadata, int *metadata_recirc,
		    void *pkt, int len, uint64_t packet_id,
		    pkt_instance_type_t instance_type) {
  queuing_pkt_t *q_pkt = malloc(sizeof(queuing_pkt_t));
  buffered_pkt_t *b_pkt = &q_pkt->pkt;
  q_pkt->metadata = metadata;
  q_pkt->metadata_recirc = metadata_recirc;
  b_pkt->instance_type = instance_type;
  b_pkt->pkt_data = pkt;
  b_pkt->pkt_len = len;
  b_pkt->pkt_id = packet_id;
  cb_write(rmt_queuing_cb, q_pkt);  
  return 0;
}
