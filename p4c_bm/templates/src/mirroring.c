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
#include "metadata_utils.h"
#include <p4utils/tommyhashlin.h>

#include <pthread.h>
#include <sys/time.h>

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"

#define MAX_HEADER_LENGTH 16
typedef struct coalescing_config_s {
  uint16_t header[MAX_HEADER_LENGTH];
  uint8_t header_length;
  uint16_t min_pkt_size;
  uint8_t timeout;
} coalescing_config_t;

#define MAX_DIGEST_SIZE 1500
typedef struct coalescing_session_s {
  coalescing_config_t config;
  uint8_t buffer[MAX_DIGEST_SIZE];
  uint32_t length;
  uint16_t sequence_number;
  struct timeval last_digest_time;
} coalescing_session_t;

typedef struct mirroring_mapping_s {
  tommy_node node;
  int mirror_id;
  int egress_port;
  coalescing_session_t *coalescing_session;
} mirroring_mapping_t;

typedef struct pkt_digest_s {
  uint8_t buffer[MAX_DIGEST_SIZE];
  uint16_t length;
  int mirror_id;
} pkt_digest_t;

#define MIRRORING_CB_SIZE 1024
static circular_buffer_t *rmt_mirroring_cb; /* only 1 */

#define MAX_COALESCING_SESSIONS 8
static uint16_t coalescing_sessions_offset = -1 * MAX_COALESCING_SESSIONS;
static coalescing_session_t coalescing_sessions[MAX_COALESCING_SESSIONS];

static tommy_hashlin mirroring_mappings;

pthread_t mirroring_thread;

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

static coalescing_session_t*
get_coalescing_session(int mirror_id) {
  mirroring_mapping_t *mapping = get_mirroring_mapping(mirror_id);
  return ((NULL == mapping) ? NULL : mapping->coalescing_session);
}

static void
begin_coalescing_session(coalescing_session_t *session) {
  uint16_t htonl_sequence_number = ++(session->sequence_number);
  memcpy(session->buffer, session->config.header, session->config.header_length);
  session->length = session->config.header_length;
  memcpy(session->buffer + session->length, &htonl_sequence_number, sizeof(htonl_sequence_number));
  session->length += sizeof(htonl_sequence_number);
  gettimeofday(&(session->last_digest_time), NULL);
}

static void
clear_coalescing_session(coalescing_session_t *session) {
  session->length = 0;
}

int ${pd_prefix}mirroring_mapping_add(p4_pd_mirror_id_t mirror_id,
                                      uint16_t egress_port
                                     )
{
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
	  "adding mirroring mapping mirror_id -> egress_port: %d -> %d\n",
	  mirror_id, egress_port);
  mirroring_mapping_t *mapping;

  mapping = tommy_hashlin_search(&mirroring_mappings,
				 compare_mirroring_mappings,
				 &mirror_id,
				 tommy_inthash_u32(mirror_id));

  if(mapping) {
    mapping->egress_port = egress_port;
  }
  else {
    mapping = malloc(sizeof(mirroring_mapping_t));
    mapping->mirror_id = mirror_id;
    mapping->egress_port = egress_port;
    mapping->coalescing_session = NULL;

    tommy_hashlin_insert(&mirroring_mappings,
			 &mapping->node,
			 mapping,
			 tommy_inthash_u32(mapping->mirror_id));
  }
  return 0;
}

int ${pd_prefix}mirror_session_create(p4_pd_sess_hdl_t shdl,
                                      p4_pd_dev_target_t dev_tgt,
                                      p4_pd_mirror_type_e type,
                                      p4_pd_direction_t dir,
                                      p4_pd_mirror_id_t mirror_id,
                                      uint16_t egress_port,
                                      uint16_t max_pkt_len,
                                      uint8_t cos,
                                      bool c2c
                                     )
{
    return ${pd_prefix}mirror_session_update(shdl, dev_tgt, type, dir,
                        mirror_id, egress_port, max_pkt_len, cos, c2c, true);
}

int ${pd_prefix}mirror_session_update(p4_pd_sess_hdl_t shdl,
                                      p4_pd_dev_target_t dev_tgt,
                                      p4_pd_mirror_type_e type,
                                      p4_pd_direction_t dir,
                                      p4_pd_mirror_id_t mirror_id,
                                      uint16_t egress_port,
                                      uint16_t max_pkt_len,
                                      uint8_t cos,
                                      bool c2c, bool enable
                                     )
{
  (void)shdl; (void)dev_tgt; (void)type; (void)dir; (void)max_pkt_len;
  (void)cos; (void)c2c, (void)enable;

  return ${pd_prefix}mirroring_mapping_add(mirror_id, egress_port);

}

int ${pd_prefix}mirroring_mapping_delete(p4_pd_mirror_id_t mirror_id) {
  mirroring_mapping_t *mapping = tommy_hashlin_remove(&mirroring_mappings,
                                 compare_mirroring_mappings,
                                 &mirror_id,
                                 tommy_inthash_u32(mirror_id));
  free(mapping);
  return (mapping == NULL); /* 0 is success */
}

int ${pd_prefix}mirror_session_delete(p4_pd_sess_hdl_t shdl,
                                      p4_pd_dev_target_t dev_tgt,
                                      p4_pd_mirror_id_t mirror_id) {
  (void)shdl; (void)dev_tgt;
  return ${pd_prefix}mirroring_mapping_delete(mirror_id);
}
int ${pd_prefix}mirroring_mapping_get_egress_port(int mirror_id) {
  mirroring_mapping_t *mapping = get_mirroring_mapping(mirror_id);
  return (mapping ? mapping->egress_port : -1);
}

int ${pd_prefix}mirroring_set_coalescing_sessions_offset(const uint16_t offset) {
  coalescing_sessions_offset = offset;
  return 0;
}

int ${pd_prefix}mirroring_add_coalescing_session(int mirror_id, int egress_port,
                                                 const int8_t *header,
                                                 const int8_t header_length,
                                                 const int16_t min_pkt_size,
                                                 const int8_t timeout) {
  mirroring_mapping_t *mapping;
  coalescing_session_t *coalescing_session;
  p4_pd_dev_target_t dev_tgt;
  dev_tgt.device_id = 0; // XXX get it as input
  dev_tgt.dev_pipe_id = PD_DEV_PIPE_ALL; // XXX get it as input

  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
          "adding coalescing session mirror_id: %d, header_length: %hd, min_pkt_size: %hd\n",
          mirror_id, header_length, min_pkt_size);

  // Sanity checks to ensure that mirror_id is in the correct range (relative to
  // coalescing_sessions_offset).
  assert(mirror_id >= coalescing_sessions_offset);
  assert(mirror_id < coalescing_sessions_offset + MAX_COALESCING_SESSIONS);

  // NEED to fix this api to get the correct info - currently not used..so
  // just getting it to compile
  ${pd_prefix}mirror_session_create(0/*shdl*/,
                                    dev_tgt,
                                    PD_MIRROR_TYPE_COAL/*type*/,
                                    PD_DIR_EGRESS,
                                    mirror_id,
                                    egress_port,
                                    0/*max_pkt_len*/,
                                    0/*cos*/,
                                    false/*c2c*/);

  mapping = get_mirroring_mapping(mirror_id);
  assert(NULL != mapping);
  mapping->coalescing_session = coalescing_sessions + mirror_id - coalescing_sessions_offset;

  coalescing_session = mapping->coalescing_session;
  coalescing_session->length = 0;
  coalescing_session->sequence_number = 0;

  assert(header_length <= MAX_HEADER_LENGTH);
  memcpy(coalescing_session->config.header, (uint8_t*)header, header_length);
  coalescing_session->config.header_length = header_length;

  coalescing_session->config.min_pkt_size = min_pkt_size;
  // FIXME: We are assuming timeout is in seconds. It should be in 'base time'.
  coalescing_session->config.timeout = timeout;

  return 0;
}

int mirroring_receive(phv_data_t *phv, int *metadata_recirc, void *pkt_data,
                      int len, uint64_t packet_id,
                      pkt_instance_type_t instance_type) {
  int mirror_id = fields_get_clone_spec(phv);
  coalescing_session_t *coalescing_session = get_coalescing_session(mirror_id);
  if(NULL != coalescing_session) {
    RMT_LOG(P4_LOG_LEVEL_VERBOSE,
      "Queuing digest for mirror %d %d\n", mirror_id, *metadata_recirc);
    pkt_digest_t *pkt_digest = malloc(sizeof(pkt_digest_t));
    pkt_digest->mirror_id = mirror_id;
    pkt_digest->length = 0;
    deparser_extract_digest(phv, metadata_recirc, pkt_digest->buffer, &(pkt_digest->length));

    cb_write(rmt_mirroring_cb, pkt_digest);
  }
  else if(NULL != get_mirroring_mapping(mirror_id)) {
    uint8_t *metadata;
    deparser_produce_metadata(phv, &metadata);
//::  if enable_intrinsic:
//::    bytes = 0
//::    for header_instance in ordered_header_instances_non_virtual:
//::      h_info = header_info[header_instance]
//::      is_metadata = h_info["is_metadata"]
//::      if is_metadata: bytes += h_info["byte_width_phv"]
//::    #endfor
     uint8_t extracted_metadata[${bytes}];
     metadata_extract(extracted_metadata, metadata, NULL); /* NULL ? */
    // do not set deflection_flag and dod bits for mirrored packet
    metadata_set_deflection_flag(extracted_metadata, 0);
    metadata_set_deflect_on_drop(extracted_metadata, 0);
    metadata_dump(metadata, extracted_metadata);
//::  #endif

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

static void mirroring_send(const int mirror_id, uint8_t *pkt_data, const uint32_t length, const uint64_t packet_id) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
          "Sending coalesced packet of length %u from coalescing session %d\n", length,
          mirror_id);

  uint8_t *metadata;
  phv_data_t *phv = phv_init(0, 0);
  fields_set_clone_spec(phv, mirror_id);
  fields_set_packet_length(phv, length);
  deparser_produce_metadata(phv, &metadata);
  free(phv);

  // Send to queuing module.
  queuing_receive(metadata, NULL, pkt_data, length, packet_id,
                  PKT_INSTANCE_TYPE_COALESCED);
}

static void *mirroring_loop(void *arg) {
  while(1) {
    const struct timeval wait_time = { .tv_sec = 1, .tv_usec = 0 };
    pkt_digest_t *pkt_digest = (pkt_digest_t *) cb_read_with_wait(rmt_mirroring_cb, &wait_time);

    struct timeval now;
    gettimeofday(&now, NULL);

    if(NULL != pkt_digest) {
      coalescing_session_t *coalescing_session = get_coalescing_session(pkt_digest->mirror_id);
      assert(NULL != coalescing_session);

      // This condition succeeds for the first digest and every digest
      // immediately after a coalesced packet is sent.
      if(0 == coalescing_session->length) {
        RMT_LOG(P4_LOG_LEVEL_VERBOSE,
                "Begin new packet for session %d\n", pkt_digest->mirror_id);
        begin_coalescing_session(coalescing_session);
      }
      const uint32_t new_length = coalescing_session->length + pkt_digest->length;

      RMT_LOG(P4_LOG_LEVEL_VERBOSE,
            "Adding %hu bytes to %u bytes in coalescing session %d\n",
            pkt_digest->length, coalescing_session->length, pkt_digest->mirror_id);
      if(new_length >= coalescing_session->config.min_pkt_size) {
        RMT_LOG(P4_LOG_LEVEL_VERBOSE,
                "Sending coalesced packet of length %u from mirror %d\n",
                coalescing_session->length + pkt_digest->length, pkt_digest->mirror_id);
        uint8_t *pkt_data = malloc(new_length);
        memcpy(pkt_data, coalescing_session->buffer, coalescing_session->length);
        memcpy(pkt_data + coalescing_session->length, pkt_digest->buffer, pkt_digest->length);

        mirroring_send(pkt_digest->mirror_id, pkt_data, new_length, coalescing_session->sequence_number);
        clear_coalescing_session(coalescing_session);
      } else {
        memcpy(coalescing_session->buffer + coalescing_session->length, pkt_digest->buffer, pkt_digest->length);
        (coalescing_session->length) += (pkt_digest->length);
      }
    }

    int i;
    for(i = 0; i < MAX_COALESCING_SESSIONS; ++i) {
      const int mirror_id = i + coalescing_sessions_offset;
      coalescing_session_t *coalescing_session = coalescing_sessions + i;
      struct timeval abs_timeout = coalescing_sessions->last_digest_time;
      abs_timeout.tv_sec += (unsigned long)coalescing_sessions->config.timeout;
      if((coalescing_session->length > 0) && !timercmp(&now, &abs_timeout, <)) {
        RMT_LOG(P4_LOG_LEVEL_VERBOSE,
                "Coalescing session timeout for mirror id %d\n", mirror_id);
        uint8_t *pkt_data = malloc(coalescing_session->length);
        memcpy(pkt_data, coalescing_session->buffer, coalescing_session->length);
        mirroring_send(mirror_id, pkt_data, coalescing_session->length,
                       coalescing_session->sequence_number);
        clear_coalescing_session(coalescing_session);
      }
    }

    free(pkt_digest);
  }
}

void mirroring_init() {
  rmt_mirroring_cb = cb_init(MIRRORING_CB_SIZE, CB_WRITE_BLOCK, CB_READ_BLOCK);

  tommy_hashlin_init(&mirroring_mappings);

  pthread_create(&mirroring_thread, NULL,
                 mirroring_loop, NULL);
}
