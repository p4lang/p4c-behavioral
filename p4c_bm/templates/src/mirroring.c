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

#include <p4_sim/mirroring.h>
#include "queuing.h"
#include "fields.h"
#include "phv.h"
#include "deparser.h"
#include <p4utils/tommyhashlin.h>

#include <pthread.h>
#include <sys/time.h>

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"

typedef struct mirroring_mapping_s {
  tommy_node node;
  int mirror_id;
  int egress_port;
} mirroring_mapping_t;

static tommy_hashlin mirroring_mappings;

static int compare_mirroring_mappings(const void* arg, const void* obj) {
  return *(int *) arg != ((mirroring_mapping_t *) obj)->mirror_id;
}

static inline mirroring_mapping_t*
get_mirroring_mapping(const int mirror_id) {
  mirroring_mapping_t *mapping;
  mapping = tommy_hashlin_search(&mirroring_mappings,
				 compare_mirroring_mappings,
				 &mirror_id,
				 tommy_inthash_u32(mirror_id));
  return mapping;
}

int ${pd_prefix}mirroring_mapping_add(int mirror_id, int egress_port){
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
	  "adding mirroring mapping mirror_id -> egress_port: %d -> %d\n",
	  mirror_id, egress_port);
  mirroring_mapping_t *mapping = malloc(sizeof(mirroring_mapping_t));
  mapping->mirror_id = mirror_id;
  mapping->egress_port = egress_port;
  tommy_hashlin_insert(&mirroring_mappings,
                       &mapping->node,
                       mapping,
                       tommy_inthash_u32(mapping->mirror_id));
  return 0;
}

int ${pd_prefix}mirroring_mapping_delete(int mirror_id) {
  mirroring_mapping_t *mapping = tommy_hashlin_remove(&mirroring_mappings,
                                 compare_mirroring_mappings,
                                 &mirror_id,
                                 tommy_inthash_u32(mirror_id));
  free(mapping);
  return (mapping == NULL); /* 0 is success */
}

int ${pd_prefix}mirroring_mapping_get_egress_port(int mirror_id) {
  mirroring_mapping_t *mapping = get_mirroring_mapping(mirror_id);
  return (mapping ? mapping->egress_port : -1);
}

int mirroring_receive(phv_data_t *phv, int *metadata_recirc, void *pkt_data,
                      int len, uint64_t packet_id,
                      pkt_instance_type_t instance_type) {
  int mirror_id = fields_get_clone_spec(phv);
  if(NULL != get_mirroring_mapping(mirror_id)) {
    uint8_t *metadata;
    deparser_produce_metadata(phv, &metadata);
    return queuing_receive(metadata, metadata_recirc, pkt_data, len, packet_id,
                           instance_type);
  }
  else {
    RMT_LOG(P4_LOG_LEVEL_WARN,
            "Received packet with invalid mirror id %d\n", mirror_id);
  }

  free(pkt_data);
  free(metadata_recirc);

  return 0;
}

void mirroring_init() {
  tommy_hashlin_init(&mirroring_mappings);
}
