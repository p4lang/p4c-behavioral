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

#ifndef _RMT_PD_STATIC_H
#define _RMT_PD_STATIC_H

#include <stdint.h>
#include <stdbool.h>

enum {
  P4_PD_SUCCESS = 0,
  P4_PD_FAILURE = 20
};

typedef uint32_t p4_pd_status_t;

typedef uint32_t p4_pd_sess_hdl_t;

typedef uint32_t p4_pd_entry_hdl_t;
typedef uint32_t p4_pd_mbr_hdl_t;
typedef uint32_t p4_pd_grp_hdl_t;
typedef uint64_t p4_pd_stat_t;

/* TODO: change that, it is horrible */
typedef void* p4_pd_value_hdl_t;

#define PD_DEV_PIPE_ALL 0xffff
typedef struct p4_pd_dev_target {
  int device_id; /*!< Device Identifier the API request is for */
  uint16_t dev_pipe_id;/*!< If specified localizes target to the resources
			 * only accessible to the port. DEV_PIPE_ALL serves
			 * as a wild-card value
			 */
} p4_pd_dev_target_t;

typedef struct p4_pd_counter_value {
  uint64_t packets;
  uint64_t bytes;
} p4_pd_counter_value_t;

p4_pd_status_t
p4_pd_init(void);

void
p4_pd_cleanup(void);

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size);

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri);

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp);

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl);

int32_t p4_pd_set_meter_time(p4_pd_sess_hdl_t shdl,
                     int32_t meter_time_disable);
uint16_t
p4_pd_dev_port_to_pipe_id(uint16_t dev_port_id);

typedef void (*p4_pd_stat_ent_sync_cb) (int device_id, void *cookie);
typedef void (*p4_pd_stat_sync_cb) (int device_id, void *cookie);

#define COUNTER_READ_HW_SYNC (1 << 0)

/* mirroring definitions */
typedef enum {
    PD_MIRROR_TYPE_NORM = 0,
    PD_MIRROR_TYPE_COAL,
    PD_MIRROR_TYPE_MAX
} p4_pd_mirror_type_e;

typedef enum {
    PD_DIR_NONE = 0,
    PD_DIR_INGRESS,
    PD_DIR_EGRESS,
    PD_DIR_BOTH
} p4_pd_direction_t;

typedef enum {
    PD_METER_TYPE_COLOR_AWARE,    /* Color aware meter */
    PD_METER_TYPE_COLOR_UNAWARE,  /* Color unaware meter */
} p4_pd_meter_type_t;

typedef enum {
    PD_INGRESS_POOL_0,
    PD_INGRESS_POOL_1,
    PD_INGRESS_POOL_2,
    PD_INGRESS_POOL_3,
    PD_EGRESS_POOL_0,
    PD_EGRESS_POOL_1,
    PD_EGRESS_POOL_2,
    PD_EGRESS_POOL_3,
} p4_pd_pool_id_t;

typedef struct p4_pd_packets_meter_spec {
  uint32_t cir_pps;
  uint32_t cburst_pkts;
  uint32_t pir_pps;
  uint32_t pburst_pkts;
  p4_pd_meter_type_t meter_type;
} p4_pd_packets_meter_spec_t;

typedef struct p4_pd_bytes_meter_spec {
  uint32_t cir_kbps;
  uint32_t cburst_kbits;
  uint32_t pir_kbps;
  uint32_t pburst_kbits;
  p4_pd_meter_type_t meter_type;
} p4_pd_bytes_meter_spec_t;

typedef uint16_t p4_pd_mirror_id_t;
#endif
