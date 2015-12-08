#ifndef _RMT_PD_PRE_H
#define _RMT_PD_PRE_H

//:: if enable_pre:

#include <p4_sim/pd.h>

/* Multicast session APIs. */
p4_pd_status_t
p4_pd_mc_create_session(p4_pd_sess_hdl_t *sess_hdl);

p4_pd_status_t
p4_pd_mc_destroy_session(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_mc_complete_operations(p4_pd_sess_hdl_t sess_hdl);

/* Multicast group APIs. */
p4_pd_status_t
p4_pd_mc_mgrp_create(p4_pd_sess_hdl_t sess_hdl, int dev, uint16_t grp,
                     p4_pd_entry_hdl_t *grp_hdl);
p4_pd_status_t
p4_pd_mc_mgrp_destroy(p4_pd_sess_hdl_t sess_hdl, int dev,
                      p4_pd_entry_hdl_t grp_hdl);

p4_pd_status_t
p4_pd_mc_node_create(p4_pd_sess_hdl_t sess_hdl, int dev, uint16_t rid,
                     uint8_t *port_map, uint8_t *lag_map,
                     p4_pd_entry_hdl_t *node_hdl);

p4_pd_status_t
p4_pd_mc_node_update(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t node_hdl,
                     uint8_t *port_map, uint8_t *lag_map);

p4_pd_status_t
p4_pd_mc_node_destroy(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t node_hdl);

p4_pd_status_t
p4_pd_mc_associate_node(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t grp_hdl,
                        p4_pd_entry_hdl_t l1_hdl);

p4_pd_status_t
p4_pd_mc_dissociate_node(p4_pd_sess_hdl_t sess_hdl, int dev,
                         p4_pd_entry_hdl_t grp_hdl,
                         p4_pd_entry_hdl_t node_hdl);

p4_pd_status_t
p4_pd_mc_set_lag_membership(p4_pd_sess_hdl_t sess_hdl, int dev, uint8_t lag,
                            uint8_t *port_map);
//:: #endif
#endif /* _RMT_PD_PRE_H */
