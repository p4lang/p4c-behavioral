//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include "mc.h"

#include <iostream>

extern "C" {
#include <p4_sim/pd_static.h>
#include <p4_sim/pd.h>
#include <p4_sim/pre.h>
}

using namespace  ::mc_pd_rpc;
using namespace  ::res_pd_rpc;

class mcHandler : virtual public mcIf {
public:
  mcHandler() {

  }

//:: if enable_pre:
  // Multicast RPC API's
  McHandle_t mc_mgrp_create(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t mgid) {
        p4_pd_entry_hdl_t pd_entry;
        ::mc_mgrp_create(sess_hdl, dev_tgt.dev_id, mgid, &pd_entry);
        return pd_entry;
  }

  int16_t mc_mgrp_destroy(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t mgid) {
      return (::mc_mgrp_destroy(sess_hdl, mgid));
  }

  McHandle_t mc_l1_node_create(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int16_t rid) {
      p4_pd_entry_hdl_t pd_entry;
      ::mc_l1_node_create(sess_hdl, rid, &pd_entry);
      return pd_entry;
  }

  int16_t mc_l1_node_destroy(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t l1_hdl) {
      return (::mc_l1_node_destroy(sess_hdl, l1_hdl));
  }

  int16_t mc_l1_associate_node(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t mgrp_hdl, const int32_t l1_hdl)  {
      return (::mc_l1_associate_node(sess_hdl, mgrp_hdl, l1_hdl));
  }

  McHandle_t mc_l2_node_create(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t l1_hdl, const std::vector<int8_t> & port_map) {
      p4_pd_entry_hdl_t pd_entry;
      ::mc_l2_node_create(sess_hdl, l1_hdl, (uint8_t *)&port_map[0], &pd_entry);
      return pd_entry;
  }

  int16_t mc_l2_node_destroy(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t l2_hdl) {
      return ::mc_l2_node_destroy(sess_hdl, l2_hdl);
  }

  int16_t mc_l2_node_update(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t l2_hdl, const std::vector<int8_t> & port_map) {
      return (::mc_l2_node_update(sess_hdl, l2_hdl, (uint8_t *)&port_map[0]));
  }
//:: #endif

};
