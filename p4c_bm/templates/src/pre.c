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

typedef struct mgrp_ {
    mgrp_id_t mgid;
    bool valid;
    tommy_list l1_list;
} mgrp_t;

typedef struct l2_node_ {
    uint8_t port_map[32];
    bool valid;
    mc_l1_node_hdl_t l1_hdl;
} l2_node_t;

typedef struct l1_node_ {
    bool valid;
    mgrp_id_t mgid;
    mgrp_rid_t rid;
    mc_l2_node_hdl_t l2_hdl;
    tommy_node node;
} l1_node_t;

/* Multicast Tables */
mgrp_t mgid_table[PRE_MGID_MAX];
l1_node_t l1_table[PRE_L1_NODE_MAX];
l2_node_t l2_table[PRE_L2_NODE_MAX];

/* Table Locks */
pthread_mutex_t session_lock;
pthread_rwlock_t l1_table_lock;
pthread_rwlock_t l2_table_lock;
pthread_rwlock_t mgid_table_lock;

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

p4_pd_status_t mc_delete_session(p4_pd_sess_hdl_t sess_hdl)
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
 */
p4_pd_status_t mc_l1_node_create(p4_pd_sess_hdl_t session,
                              mgrp_rid_t rid,
                              mc_l1_node_hdl_t *l1_hdl)
{
    l1_node_t            *l1_node = NULL;
    int                   l1_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);
    l1_index = pre_id_allocate(&l1_bmap_allocator);
    l1_node = &l1_table[l1_index];
    l1_node->rid = rid;
    l1_node->valid = TRUE;

    *l1_hdl = L1_TO_HANDLE(l1_index);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Associate L1 Node to multicast tree
 @param session - session handle
 @param mgrp_hdl - Multicast Group Handle
 @param l1_hdl - L1 Node Handle
 */
p4_pd_status_t mc_l1_associate_node(p4_pd_sess_hdl_t session,
                                 mc_mgrp_hdl_t mgrp_hdl,
                                 mc_l1_node_hdl_t l1_hdl)
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
 Delete L1 Multicast Node
 @param session - session handle
 @param l1_hdl - L1 node handle
 */
p4_pd_status_t mc_l1_node_destroy(p4_pd_sess_hdl_t session,
                           mc_l1_node_hdl_t l1_hdl)
{
    l1_node_t              *l1_node = NULL;
    int                     l1_index = 0;
    mgrp_t                 *mgrp = NULL;

    pthread_rwlock_wrlock(&mgid_table_lock);
    pthread_rwlock_wrlock(&l1_table_lock);
    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];

    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l1_table_lock);
        pthread_rwlock_unlock(&mgid_table_lock);
        return -1;
    }

    mgrp = &mgid_table[l1_node->mgid];
    l1_node = tommy_list_remove_existing(&(mgrp->l1_list), &(l1_node->node));
    memset(l1_node, 0, sizeof(l1_node_t));
    pre_id_release(&l1_bmap_allocator, l1_index);
    pthread_rwlock_unlock(&l1_table_lock);
    pthread_rwlock_unlock(&mgid_table_lock);
    return 0;
}

/*
 Create Level 2 multicast node
 @param session - session handle
 @param l1_hdl - L1 Node handle
 @param port_map - List of ports
 */
p4_pd_status_t mc_l2_node_create(p4_pd_sess_hdl_t session,
                              mc_l1_node_hdl_t l1_hdl,
                              const uint8_t *port_map,
                              mc_l2_node_hdl_t *l2_hdl)
{
    l1_node_t          *l1_node = NULL;
    l2_node_t          *l2_node = NULL;
    int                 l1_index = 0;
    int                 l2_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);
    pthread_rwlock_wrlock(&l2_table_lock);
    l1_index = HANDLE_TO_L1(l1_hdl);
    l1_node = &l1_table[l1_index];
    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }
    
    l2_index = pre_id_allocate(&l2_bmap_allocator);
    l2_node = &l2_table[l2_index];
    l2_node->valid = TRUE;

    memcpy(l2_node->port_map, port_map, 32);

    *l2_hdl = L2_TO_HANDLE(l2_index);
    l2_node->l1_hdl = l1_hdl;
    l1_node->l2_hdl = *l2_hdl;
    pthread_rwlock_unlock(&l2_table_lock);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Delete Level 2 multicast node
 @param session - session handle
 @param l2_hdl - L2 Node handle
 */
p4_pd_status_t mc_l2_node_destroy(p4_pd_sess_hdl_t session,
                               mc_l2_node_hdl_t l2_hdl)
{
    l1_node_t           *l1_node = NULL;
    l2_node_t           *l2_node = NULL;
    int                  l1_index = 0;
    int                  l2_index = 0;

    pthread_rwlock_wrlock(&l1_table_lock);
    pthread_rwlock_wrlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l2_hdl);
    l2_node = &l2_table[l2_index];
    if (!l2_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    l1_index = HANDLE_TO_L1(l2_node->l1_hdl);
    l1_node = &l1_table[l1_index];
    if (!l1_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        pthread_rwlock_unlock(&l1_table_lock);
        return -1;
    }

    memset(l2_node, 0, sizeof(l2_node_t));
    // Clear the l2_hdl of l1 node only when they
    // match. The reason is the application can just
    // destroy the l2_node but reuse and point it to
    // different l2_node;
    if (l1_node->l2_hdl == l2_hdl) {
        l1_node->l2_hdl = 0;
    }
    pre_id_release(&l2_bmap_allocator, l2_index);
    pthread_rwlock_unlock(&l2_table_lock);
    pthread_rwlock_unlock(&l1_table_lock);
    return 0;
}

/*
 Update Level 2 multicast node
 @param l2_hdl - Level 2 Node handle
 @param port_map - List of ports
 */
p4_pd_status_t mc_l2_node_update(p4_pd_sess_hdl_t session,
                              mc_l2_node_hdl_t l2_hdl,
                              const uint8_t *port_map)
{
    l2_node_t             *l2_node = NULL;
    int                    l2_index = 0;

    pthread_rwlock_wrlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l2_hdl);
    l2_node = &l2_table[l2_index];
    if (!l2_node->valid) {
        pthread_rwlock_unlock(&l2_table_lock);
        return -1;
    }

    memcpy(l2_node->port_map, port_map, 32);
    pthread_rwlock_unlock(&l2_table_lock);
    return 0;
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
    l2_node_t          *l2_node = NULL;
    bool               port_is_set = FALSE;
    int                i = 0, j = 0;
    int                l2_index = 0;
    int                metadata_recirc_length = 0;
    uint8_t            *q_metadata_copy = NULL;
    uint8_t            *pkt_data_copy = NULL;
    int                *metadata_recirc_copy = NULL;

    pthread_rwlock_rdlock(&l2_table_lock);
    l2_index = HANDLE_TO_L2(l2_hdl);
    l2_node = &l2_table[l2_index];

    metadata_recirc_length = mc_compute_metadata_recirc_length(metadata_recirc);

    memcpy(local_metadata, metadata, LOCAL_METADATA_LENGTH);
    /*
     * metadata, metada_recirc and pkt_data are allocated for
     * every packet that is replicated. It is freed in the egress
     * pipeline. All we need to do is free the original packet once
     * done with the replication.
     */
    /*
     * TODO: Remove hardcoded values and include appropriate
     * defines for max ports
     */
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 8; j++) {
            port_is_set = l2_node->port_map[i] & (1 << j); 
            if (port_is_set) {
                port_id = (i * 8) + (j + 1);
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
