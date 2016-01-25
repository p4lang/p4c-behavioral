//:: if enable_pre:
#include <p4_sim/pd_pre.h>
#include <p4_sim/pre.h>

p4_pd_status_t
p4_pd_mc_create_session(p4_pd_sess_hdl_t *sess_hdl)
{
    return mc_create_session(sess_hdl);
}

p4_pd_status_t
p4_pd_mc_destroy_session(p4_pd_sess_hdl_t sess_hdl)
{
    return mc_destroy_session(sess_hdl);
}

p4_pd_status_t
p4_pd_mc_complete_operations(p4_pd_sess_hdl_t sess_hdl)
{
    return mc_complete_operations(sess_hdl);
}

p4_pd_status_t
p4_pd_mc_mgrp_create(p4_pd_sess_hdl_t sess_hdl, int dev, uint16_t grp,
                     p4_pd_entry_hdl_t *grp_hdl)
{
    return mc_mgrp_create(sess_hdl, dev, grp, grp_hdl);
}

p4_pd_status_t
p4_pd_mc_mgrp_destroy(p4_pd_sess_hdl_t sess_hdl, int dev,
                      p4_pd_entry_hdl_t grp_hdl)
{
    return mc_mgrp_destroy(sess_hdl, grp_hdl);
}

p4_pd_status_t
p4_pd_mc_node_create(p4_pd_sess_hdl_t sess_hdl, int dev, uint16_t rid,
                     uint8_t *port_map, uint8_t *lag_map,
                     p4_pd_entry_hdl_t *node_hdl)
{
    return mc_node_create(sess_hdl, dev, rid, port_map, lag_map, node_hdl);
}

p4_pd_status_t
p4_pd_mc_node_update(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t node_hdl,
                     uint8_t *port_map, uint8_t *lag_map)
{
    return mc_node_update(sess_hdl, node_hdl, port_map, lag_map);
}

p4_pd_status_t
p4_pd_mc_node_destroy(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t node_hdl)
{
    return mc_node_destroy(sess_hdl, node_hdl);
}

p4_pd_status_t
p4_pd_mc_associate_node(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t grp_hdl,
                        p4_pd_entry_hdl_t l1_hdl, uint16_t xid, bool xid_valid)
{
    (void) xid;
    (void) xid_valid;
    return mc_associate_node(sess_hdl, grp_hdl, l1_hdl);
}

p4_pd_status_t
p4_pd_mc_dissociate_node(p4_pd_sess_hdl_t sess_hdl, int dev, p4_pd_entry_hdl_t grp_hdl,
                         p4_pd_entry_hdl_t node_hdl)
{
    return mc_dissociate_node(sess_hdl, grp_hdl, node_hdl);
}

p4_pd_status_t
p4_pd_mc_set_lag_membership(p4_pd_sess_hdl_t sess_hdl, int dev, uint8_t lag,
                            uint8_t *port_map)
{
    return mc_set_lag_membership(sess_hdl, dev, lag, port_map);
}

p4_pd_status_t
p4_pd_mc_ecmp_create(p4_pd_sess_hdl_t session,
                     int device,
                     p4_pd_entry_hdl_t *ecmp_hdl) {
  (void) session; (void) device; (void) ecmp_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_destroy(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl) {
  (void) session; (void) device; (void) ecmp_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_mbr_add(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl) {
  (void) session; (void) device; (void) ecmp_hdl; (void) l1_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_ecmp_mbr_rem(p4_pd_sess_hdl_t session,
                      int device,
                      p4_pd_entry_hdl_t ecmp_hdl,
                      p4_pd_entry_hdl_t l1_hdl) {
  (void) session; (void) device; (void) ecmp_hdl; (void) l1_hdl;
  return 0;
}

p4_pd_status_t
p4_pd_mc_associate_ecmp(p4_pd_sess_hdl_t session,
                        int device,
                        p4_pd_entry_hdl_t grp_hdl,
                        p4_pd_entry_hdl_t ecmp_hdl,
                        uint16_t xid, bool xid_valid) {
  (void) session; (void) device; (void) grp_hdl; (void) ecmp_hdl;
  (void) xid_valid;
  return 0;
}

p4_pd_status_t
p4_pd_mc_dissociate_ecmp(p4_pd_sess_hdl_t session,
                         int device,
                         p4_pd_entry_hdl_t grp_hdl,
                         p4_pd_entry_hdl_t ecmp_hdl) {
  (void) session; (void) device; (void) grp_hdl; (void) ecmp_hdl;
  return 0;
}

//:: #endif
