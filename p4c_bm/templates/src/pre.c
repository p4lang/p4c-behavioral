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

//:: if enable_pre:
#include <pthread.h>

#include "queuing.h"
#include "parser.h"
#include "deparser.h"
#include "enums.h"
#include "pipeline.h"
#include "metadata_recirc.h"
#include "metadata_utils.h"
#include <p4utils/tommylist.h>
#include <p4_sim/pd.h>
#include <p4_sim/pre.h>

#define TRUE 1
#define FALSE 0

#define PRE_PORT_MAP_ARRAY_SIZE ((PRE_PORTS_MAX)/8)
#define PRE_LAG_MAP_ARRAY_SIZE ((PRE_LAG_MAX)/8)

typedef struct mgrp_ {
    mgrp_id_t mgid;
    bool valid;
    tommy_list l1_list;
} mgrp_t;

typedef uint32_t mc_l2_node_hdl_t;
typedef struct l2_node_ {
    uint8_t lag_map[PRE_LAG_MAP_ARRAY_SIZE];
    uint8_t port_map[PRE_PORT_MAP_ARRAY_SIZE];
    bool valid;
    mc_node_hdl_t l1_hdl;
} l2_node_t;

typedef struct l1_node_ {
    bool valid;
    mgrp_id_t mgid;
    mgrp_rid_t rid;
    mc_l2_node_hdl_t l2_hdl;
    tommy_node node;
} l1_node_t;

typedef struct lag_ {
    uint8_t port_map[PRE_PORT_MAP_ARRAY_SIZE];
    uint32_t port_count;
} lag_t;

/* Multicast Tables */
mgrp_t mgid_table[PRE_MGID_MAX];
l1_node_t l1_table[PRE_L1_NODE_MAX];
l2_node_t l2_table[PRE_L2_NODE_MAX];
lag_t lag_table[PRE_LAG_MAX];

/* Table Locks */
pthread_mutex_t session_lock;
pthread_rwlock_t l1_table_lock;
pthread_rwlock_t l2_table_lock;
pthread_rwlock_t mgid_table_lock;
pthread_rwlock_t lag_table_lock;

Pvoid_t session_id_allocator = (Pvoid_t) NULL;
Pvoid_t l1_bmap_allocator = (Pvoid_t) NULL;
Pvoid_t l2_bmap_allocator = (Pvoid_t) NULL;

uint8_t *local_metadata = NULL;

#define handle_to_id(mgid_handle) \
    (mgid_handle & 0xFFFF)

#define MGID_TO_HANDLE(mgid) \
    ((1 << 28) | mgid)

#define HANDLE_TO_MGID(mgid_handle) \
    handle_to_id(mgid_handle)

#define L1_TO_HANDLE(l1_id) \
    ((2 << 28) | l1_id)

#define L2_TO_HANDLE(l2_id) \
    ((3 << 28) | l2_id)

#define HANDLE_TO_L1(l1_hdl) \
    handle_to_id(l1_hdl)

#define HANDLE_TO_L2(l2_hdl) \
    handle_to_id(l2_hdl)

//:: q_metadata_length = 0
//:: for header_instance in ordered_header_instances_metadata:
//::   h_info = header_info[header_instance]
//::   q_metadata_length += h_info["bit_width"] / 8
//:: #endfor  

//:: local_metadata_length = 0
//:: for header_instance in ordered_header_instances_non_virtual:
//::   h_info = header_info[header_instance]
//::   is_metadata = h_info["is_metadata"]
//::   if is_metadata: local_metadata_length += h_info["byte_width_phv"]
//:: #endfor
//

#define LOCAL_METADATA_LENGTH ${local_metadata_length}
#define Q_METADATA_LENGTH ${q_metadata_length}

void mc_pre_init()
{
    local_metadata = malloc(LOCAL_METADATA_LENGTH);
    pthread_mutex_init(&session_lock, NULL);
    pthread_rwlock_init(&l1_table_lock, NULL);
    pthread_rwlock_init(&l2_table_lock, NULL);
    pthread_rwlock_init(&mgid_table_lock, NULL);
}

int pre_id_allocate(Pvoid_t *id_allocator)
{
    Word_t index = 0;
    int Rc_int;
    J1FE(Rc_int, *id_allocator, index);
    J1S(Rc_int, *id_allocator, index);
    return index;
}

void pre_id_release(Pvoid_t *id_allocator, int index)
{
    int Rc_int;
    J1U(Rc_int, *id_allocator, index);
}

int pre_id_set(Pvoid_t id_allocator, int index)
{
    int Rc_int;
    J1T(Rc_int, id_allocator, index);
    return Rc_int;
}

p4_pd_status_t mc_create_session(p4_pd_sess_hdl_t *sess_hdl)
{
    pthread_mutex_lock(&session_lock);
    int Rc_int;
    Word_t index = 0;

    J1FE(Rc_int, session_id_allocator, index);
    *sess_hdl = (p4_pd_sess_hdl_t)index;
    J1S(Rc_int, session_id_allocator, index);
    assert(1 == Rc_int);
    pthread_mutex_unlock(&session_lock);
    return 0;
}

p4_pd_status_t mc_destroy_session(p4_pd_sess_hdl_t sess_hdl)
{
    pthread_mutex_lock(&session_lock);
    int Rc_int;
    J1U(Rc_int, session_id_allocator, (Word_t)sess_hdl);
    if (0 == Rc_int) {
        RMT_LOG(P4_LOG_LEVEL_ERROR, "Cannot find session handle %u\n", (uint32_t)sess_hdl);
    }
    pthread_mutex_unlock(&session_lock);
    return (p4_pd_status_t)(1 == Rc_int ? 0 : 1);
}

/*
 Create a Multicast Group
 @param session - session handle
 @param mgid - Multicast Index
 */
p4_pd_status_t mc_mgrp_create(p4_pd_sess_hdl_t session,
                              int8_t device,
                              mgrp_id_t mgid,
                              mc_mgrp_hdl_t *mgrp_hdl)
{
    mgrp_t              *mgrp = NULL;

    pthread_rwlock_wrlock(&mgid_table_lock);
    mgrp = &mgid_table[mgid];
    if (mgrp->valid) {
        pthread_rwlock_unlock(&mgid_table_lock);
        return 0;
    }

    mgrp->valid = TRUE;
    tommy_list_init(&(mgrp->l1_list));
    *mgrp_hdl = MGID_TO_HANDLE(mgid);
    pthread_rwlock_unlock(&mgid_table_lock);
    return 0;
}

/*
 Delete a Multicast Group
 @param session - session handle
 @param mgrp_hdl - Multicast Group Handle
 */
p4_pd_status_t mc_mgrp_destroy(p4_pd_sess_hdl_t session,
                               mc_mgrp_hdl_t mgrp_hdl)
{
    mgrp_id_t        mgid = 0;
    mgrp_t          *mgrp = NULL;

    pthread_rwlock_wrlock(&mgid_table_lock);
    mgid = HANDLE_TO_MGID(mgrp_hdl);
    mgrp = &mgid_table[mgid];
    mgrp->valid = FALSE;
    pthread_rwlock_unlock(&mgid_table_lock);
    return 0;
}

/*
 Create L1 Multicast Node
 @param session - session handle
 @param rid - Replication ID
 @param port_map - List of member ports
 @param lag_map - List of LAGs
 @param l1_hdl - L1 Node handle
 */
p4_pd_status_t mc_node_create(p4_pd_sess_hdl_t session,
                              int8_t device,
                              mgrp_rid_t rid,
                              const uint8_t *port_map,
                              const uint8_t *lag_map,
                              mc_node_hdl_t *l1_hdl)
{
    mc_l2_node_hdl_t      l2_hdl;
    l1_node_t            *l1_node = NULL;
    l2_node_t            *l2_node = NULL;
    int                   l1_index = 0;
    int                   l2_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);
    pthread_rwlock_wrlock(&l2_table_lock);

    l1_index = pre_id_allocate(&l1_bmap_allocator);
    l1_node = &l1_table[l1_index];
    l1_node->rid = rid;
    l1_node->valid = TRUE;

    l2_index = pre_id_allocate(&l2_bmap_allocator);
    l2_node = &l2_table[l2_index];
    l2_node->valid = TRUE;

    memcpy(l2_node->port_map, port_map, PRE_PORT_MAP_ARRAY_SIZE);
    memcpy(l2_node->lag_map, lag_map, PRE_LAG_MAP_ARRAY_SIZE);

    *l1_hdl = L1_TO_HANDLE(l1_index);
    l2_hdl = L2_TO_HANDLE(l2_index);
    l1_node->l2_hdl = l2_hdl;
    l2_node->l1_hdl = *l1_hdl;

    pthread_rwlock_unlock(&l2_table_lock);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Delete Multicast Node
 @param session - session handle
 @param l1_hdl - node handle
 */
p4_pd_status_t mc_node_destroy(p4_pd_sess_hdl_t session,
                               mc_node_hdl_t l1_hdl)
{
    l1_node_t              *l1_node = NULL;
    l2_node_t              *l2_node = NULL;
    int                     l1_index = 0;
    int                     l2_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);
    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];

    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    pthread_rwlock_wrlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l1_node->l2_hdl);
    l2_node = &l2_table[l2_index];
    if (!l2_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    memset(l2_node, 0, sizeof(l2_node_t));
    l1_node->l2_hdl = 0;
    pre_id_release(&l2_bmap_allocator, l2_index);
    pthread_rwlock_unlock(&l2_table_lock);

    memset(l1_node, 0, sizeof(l1_node_t));
    pre_id_release(&l1_bmap_allocator, l1_index);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Update L1 Multicast Node
 @param session - session handle
 @param l1_hdl - L1 Node handle
 @param port_map - List of member ports, replaces existing members
 @param lag_map - List of LAGs
 */
p4_pd_status_t mc_node_update(p4_pd_sess_hdl_t session,
                              mc_node_hdl_t l1_hdl,
                              const uint8_t *port_map,
                              const uint8_t *lag_map)
{
    l1_node_t            *l1_node = NULL;
    l2_node_t            *l2_node = NULL;
    int                   l1_index = 0;
    int                   l2_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);

    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];

    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    pthread_rwlock_wrlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l1_node->l2_hdl);
    l2_node = &l2_table[l2_index];
    if (!l2_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    memcpy(l2_node->port_map, port_map, PRE_PORT_MAP_ARRAY_SIZE);
    memcpy(l2_node->lag_map, lag_map, PRE_LAG_MAP_ARRAY_SIZE);

    pthread_rwlock_unlock(&l2_table_lock);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Associate Node to multicast tree
 @param session - session handle
 @param mgrp_hdl - Multicast Group Handle
 @param l1_hdl - Node Handle
 @param xid_valid - xid valid indicator
 @param xid - Level 1 Exclusion ID
 */
p4_pd_status_t mc_associate_node(p4_pd_sess_hdl_t session,
                                 mc_mgrp_hdl_t mgrp_hdl,
                                 mc_node_hdl_t l1_hdl)
{
    mgrp_id_t        mgid = 0;
    int              l1_index = 0;
    mgrp_t          *mgrp = NULL;
    l1_node_t       *l1_node = NULL;

    pthread_rwlock_wrlock(&mgid_table_lock);
    pthread_rwlock_wrlock(&l1_table_lock);
    mgid = HANDLE_TO_MGID(mgrp_hdl);
    mgrp = &mgid_table[mgid];
    if (!mgrp->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];
    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    l1_node->mgid = mgid;

    tommy_list_insert_head(&mgrp->l1_list, &(l1_node->node), l1_node);
    pthread_rwlock_unlock(&l1_table_lock);
    pthread_rwlock_unlock(&mgid_table_lock);
    return 0;
}

/*
 Dissociate Node from multicast tree
 @param session - session handle
 @param mgrp_hdl - Multicast Group Handle
 @param l1_hdl - Node Handle
 */
p4_pd_status_t mc_dissociate_node(p4_pd_sess_hdl_t session,
                                  mc_mgrp_hdl_t mgrp_hdl,
                                  mc_node_hdl_t l1_hdl)
{
    mgrp_id_t        mgid = 0;
    int              l1_index = 0;
    mgrp_t          *mgrp = NULL;
    l1_node_t       *l1_node = NULL;

    pthread_rwlock_wrlock(&mgid_table_lock);
    pthread_rwlock_wrlock(&l1_table_lock);
    mgid = HANDLE_TO_MGID(mgrp_hdl);
    mgrp = &mgid_table[mgid];
    if (!mgrp->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];
    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    if (l1_node->mgid != mgid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    l1_node = tommy_list_remove_existing(&(mgrp->l1_list), &(l1_node->node));
    pthread_rwlock_unlock(&mgid_table_lock);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}


/*
 Update Lag table
 @param session - session handle
 @param lag_id - Lag Index
 @param port_map - List of member ports
 */
p4_pd_status_t mc_set_lag_membership(p4_pd_sess_hdl_t session,
                                     int8_t dev_id,
                                     mgrp_lag_id_t lag_id,
                                     const uint8_t *port_map)
{
    lag_t        *lag = NULL;
    bool          port_set = FALSE;
    int           i = 0, j = 0;

    pthread_rwlock_wrlock(&lag_table_lock);
    lag = &lag_table[lag_id];
    memcpy(lag->port_map, port_map, PRE_PORT_MAP_ARRAY_SIZE);
    lag->port_count = 0;
    for (i = 0; i < PRE_PORT_MAP_ARRAY_SIZE; i++) {
        for (j = 0; j < 8; j++) {
            port_set = (lag->port_map[i] >> j) & 0x1;
            if (port_set) {
                lag->port_count++;
            }
        }
    }
    pthread_rwlock_unlock(&lag_table_lock);
    return 0;
}

p4_pd_status_t mc_complete_operations(p4_pd_sess_hdl_t session)
{
    return 0;
}

/*
 Returns a member port based on L2 Hash
 @param lag_index- Lag Index
 */
static int mc_lag_compute_hash(uint16_t lag_hash, mgrp_lag_id_t lag_index)
{
    uint8_t              port_count1 = 0;
    uint8_t              port_count2 = 0;
    lag_t               *lag_entry = NULL;
    int                  i = 0, j = 0;
    bool                 port_set = FALSE;
    mgrp_port_id_t       port_id = 0;

    lag_entry = &lag_table[lag_index];
    port_count1 = lag_hash % lag_entry->port_count + 1;
    for (i = 0; i < PRE_PORT_MAP_ARRAY_SIZE; i++) {
        for (j = 0; j < 8; j++) {
            port_set = (lag_entry->port_map[i] >> j) & 0x1;
            if (port_set) {
                port_count2++;
                if (port_count1 == port_count2) {
                    port_id = (i * 8) + j;
                    break;
                }
            }
        }
    }
    return port_id;
}

static int mc_compute_metadata_recirc_length(int *metadata_recirc)
{
    int length = 0;
    if (metadata_recirc) {
        length = (*metadata_recirc + 1) * sizeof(int);
    }
    return length;
}

/*
 Check for pruning and replicates packet for every L2 Node
 @param l2_hdl - Level 2 Node handle
 @param rid - Replication ID that has to be attached to egress
 @param q_metadata - queuing metadata
 @param b_pkt - Buffered Packet
 */
static void mc_replicate_packet(mc_l2_node_hdl_t l2_hdl, mgrp_rid_t rid,
                                uint8_t *metadata, int *metadata_recirc,
                                buffered_pkt_t *b_pkt)
{
    mgrp_port_id_t     port_id = 0;
    mgrp_port_id_t     lag_port_id = 0;
    mgrp_lag_id_t      lag_index = 0;
    l2_node_t          *l2_node = NULL;
    bool               port_is_set = FALSE;
    bool               lag_is_set = FALSE;
    int                i = 0, j = 0;
    int                l2_index = 0;
    int                metadata_recirc_length = 0;
    uint8_t            *q_metadata_copy = NULL;
    uint8_t            *pkt_data_copy = NULL;
    int                *metadata_recirc_copy = NULL;
    uint16_t            lag_hash = 0;

    pthread_rwlock_rdlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l2_hdl);
    l2_node = &l2_table[l2_index];

    metadata_recirc_length = mc_compute_metadata_recirc_length(metadata_recirc);

    memcpy(local_metadata, metadata, LOCAL_METADATA_LENGTH);
    lag_hash = metadata_get_lag_hash(local_metadata);
    /*
     * metadata, metada_recirc and pkt_data are allocated for
     * every packet that is replicated. It is freed in the egress
     * pipeline. All we need to do is free the original packet once
     * done with the replication.
     */
    for (i = 0; i < PRE_PORT_MAP_ARRAY_SIZE; i++) {
        for (j = 0; j < 8; j++) {
            port_is_set = l2_node->port_map[i] & (1 << j); 
            if (port_is_set) {
                port_id = (i * 8) + j;
                q_metadata_copy = malloc(Q_METADATA_LENGTH);

                if (metadata_recirc_length) {
                    metadata_recirc_copy = malloc(metadata_recirc_length);
                    memcpy(metadata_recirc_copy, metadata_recirc, metadata_recirc_length);
                }

                pkt_data_copy = malloc(b_pkt->pkt_len);
                memcpy(pkt_data_copy, b_pkt->pkt_data, b_pkt->pkt_len);

                metadata_set_egress_port(local_metadata, port_id);
                metadata_set_replication_id(local_metadata, rid);
                metadata_dump(q_metadata_copy, local_metadata);

                egress_pipeline_receive(port_id, q_metadata_copy,
                                        metadata_recirc_copy, pkt_data_copy,
                                        b_pkt->pkt_len, b_pkt->pkt_id,
                                        PKT_INSTANCE_TYPE_REPLICATION);
            }
        }
    }
    for (i = 0; i < PRE_LAG_MAP_ARRAY_SIZE; i++) {
        for (j = 0; j < 8; j++) {
            lag_is_set = l2_node->lag_map[i] & (1 << j); 
            if (lag_is_set) {
                lag_index = (i * 8) + j;
                lag_port_id = mc_lag_compute_hash(lag_hash, lag_index);
                q_metadata_copy = malloc(Q_METADATA_LENGTH);

                if (metadata_recirc_length) {
                    metadata_recirc_copy = malloc(metadata_recirc_length);
                    memcpy(metadata_recirc_copy, metadata_recirc, metadata_recirc_length);
                }

                pkt_data_copy = malloc(b_pkt->pkt_len);
                memcpy(pkt_data_copy, b_pkt->pkt_data, b_pkt->pkt_len);

                metadata_set_egress_port(local_metadata, lag_port_id);
                metadata_set_replication_id(local_metadata, rid);
                metadata_dump(q_metadata_copy, local_metadata);

                egress_pipeline_receive(lag_port_id, q_metadata_copy,
                                        metadata_recirc_copy, pkt_data_copy,
                                        b_pkt->pkt_len, b_pkt->pkt_id,
                                        PKT_INSTANCE_TYPE_REPLICATION);
            }
        }
    }
    pthread_rwlock_unlock(&l2_table_lock);
}

/*
 Process the multicast index and replicates the packet
 @param mgid - Multicast Index
 @param q_metadata - queuing metadata
 @param b_pkt - Buffered Packet
 */
void mc_process_and_replicate(uint16_t mgid, uint8_t *metadata,
                              int *metadata_recirc,
                              buffered_pkt_t *b_pkt)
{
    mgrp_t             *mgrp = NULL;
    tommy_node         *node = NULL;
    l1_node_t          *l1_node = NULL;

    if (!mgid) {
        return;
    }

    pthread_rwlock_rdlock(&mgid_table_lock);
    mgrp = &mgid_table[mgid];
    if (!mgrp->valid) {
        // Drop the packet. Invalid MGID
        pthread_rwlock_unlock(&mgid_table_lock);
        return;
    }

    node = tommy_list_head(&mgrp->l1_list);
    while (node) {
        l1_node = node->data;
        mc_replicate_packet(l1_node->l2_hdl, l1_node->rid, metadata, metadata_recirc, b_pkt);
        node = node->next;
    }
    pthread_rwlock_unlock(&mgid_table_lock);
    return;
}
//:: #endif
