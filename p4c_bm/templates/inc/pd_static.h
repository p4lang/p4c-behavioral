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

typedef uint32_t p4_pd_status_t;

typedef uint32_t p4_pd_sess_hdl_t;

typedef uint32_t p4_pd_entry_hdl_t;
typedef uint32_t p4_pd_mbr_hdl_t;
typedef uint32_t p4_pd_grp_hdl_t;
/* TODO: change that, it is horrible */
typedef void* p4_pd_value_hdl_t;

#define PD_DEV_PIPE_ALL 0xffff
typedef struct p4_pd_dev_target {
  uint8_t device_id; /*!< Device Identifier the API request is for */
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

typedef uint16_t p4_pd_mirror_id_t;

#endif
