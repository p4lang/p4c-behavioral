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

#ifndef _RMT_PRE_H
#define _RMT_PRE_H

//:: if enable_pre:

#include <p4_sim/pd.h>

#define PRE_MGID_MAX 16384
#define PRE_PORTS_MAX 256
#define PRE_L1_NODE_MAX 16384
#define PRE_L2_NODE_MAX 4096
#define PRE_LAG_MAX 256

typedef uint16_t mgrp_id_t;
typedef uint16_t mgrp_rid_t;
typedef uint16_t mgrp_lag_id_t;
typedef uint16_t mgrp_port_id_t;

typedef uint32_t mc_mgrp_hdl_t;
typedef uint32_t mc_node_hdl_t;

p4_pd_status_t
mc_create_session(p4_pd_sess_hdl_t *sess_hdl);

p4_pd_status_t
mc_destroy_session(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
mc_mgrp_create(p4_pd_sess_hdl_t session,
               int8_t device,
               mgrp_id_t mgid,
               mc_mgrp_hdl_t *mgrp_hdl);

p4_pd_status_t
mc_mgrp_destroy(p4_pd_sess_hdl_t session,
                mc_mgrp_hdl_t mgrp_hdl);

p4_pd_status_t
mc_node_create(p4_pd_sess_hdl_t session,
               int8_t device,
               mgrp_rid_t rid,
               const uint8_t *port_map,
               const uint8_t *lag_map,
               mc_node_hdl_t *node_hdl);

p4_pd_status_t
mc_node_destroy(p4_pd_sess_hdl_t session,
                mc_node_hdl_t node_hdl);

p4_pd_status_t
mc_node_update(p4_pd_sess_hdl_t session,
               mc_node_hdl_t node_hdl,
               const uint8_t *port_map,
               const uint8_t *lag_map);

p4_pd_status_t
mc_associate_node(p4_pd_sess_hdl_t session,
                  mc_mgrp_hdl_t mgrp_hdl,
                  mc_node_hdl_t node_hdl);

p4_pd_status_t
mc_dissociate_node(p4_pd_sess_hdl_t session,
                  mc_mgrp_hdl_t mgrp_hdl,
                  mc_node_hdl_t node_hdl);

p4_pd_status_t
mc_set_lag_membership(p4_pd_sess_hdl_t session,
                      int8_t dev_id,
                      mgrp_lag_id_t lag_id,
                      const uint8_t *port_map);

p4_pd_status_t
mc_complete_operations(p4_pd_sess_hdl_t shdl);

//:: #endif
#endif /* _RMT_PRE_H */
