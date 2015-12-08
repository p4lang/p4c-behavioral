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

  int32_t mc_init(){
    return 0;
  }

  SessionHandle_t mc_create_session(){
    SessionHandle_t sess_hdl;
    ::mc_create_session((uint32_t*)&sess_hdl);
    return sess_hdl;
  }

  int32_t mc_destroy_session(const SessionHandle_t sess_hdl){
    return ::mc_destroy_session(sess_hdl);
  }

  McHandle_t mc_mgrp_create(const SessionHandle_t sess_hdl, const int dev, const int16_t mgid){
    McHandle_t grp_hdl;
    ::mc_mgrp_create(sess_hdl, dev, mgid, (uint32_t*)&grp_hdl);
    return grp_hdl;
  }

  int32_t mc_mgrp_destroy(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl){
    return ::mc_mgrp_destroy(sess_hdl, grp_hdl);
  }

  McHandle_t mc_node_create(const SessionHandle_t sess_hdl, const int dev, const int16_t rid, const std::string &port_map, const std::string &lag_map){
    McHandle_t l1_hdl;
    ::mc_node_create(sess_hdl, dev, rid, (uint8_t*)port_map.c_str(), (uint8_t*)lag_map.c_str(), (uint32_t*)&l1_hdl);
    return l1_hdl;
  }

  McHandle_t mc_node_update(const SessionHandle_t sess_hdl, const int dev,  const McHandle_t node_hdl, const std::string &port_map, const std::string &lag_map){
    return ::mc_node_update(sess_hdl, node_hdl, (uint8_t*)port_map.c_str(), (uint8_t*)lag_map.c_str());
  }

  int32_t mc_node_destroy(const SessionHandle_t sess_hdl, const int dev, const McHandle_t node_hdl){
    return ::mc_node_destroy(sess_hdl, node_hdl);
  }

  int32_t mc_associate_node(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl, const McHandle_t l1_hdl){
    return ::mc_associate_node(sess_hdl, grp_hdl, l1_hdl);
  }

  int32_t mc_dissociate_node(const SessionHandle_t sess_hdl, const int dev, const McHandle_t grp_hdl, const McHandle_t l1_hdl){
    return ::mc_dissociate_node(sess_hdl, grp_hdl, l1_hdl);
  }

  int32_t mc_set_lag_membership(const SessionHandle_t sess_hdl, const int dev, const int8_t lag, const std::string &port_map){
    int32_t y = ::mc_set_lag_membership(sess_hdl, dev, lag, (uint8_t*)port_map.c_str());
    return y;
  }

//:: #endif

};
