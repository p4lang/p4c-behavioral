//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include "conn_mgr.h"

#include <iostream>

extern "C" {
#include <p4_sim/pd_static.h>
#include <p4_sim/rmt.h>
#include <p4_sim/pg.h>    
}

using namespace  ::conn_mgr_pd_rpc;
using namespace  ::res_pd_rpc;

class conn_mgrHandler : virtual public conn_mgrIf {
public:
  conn_mgrHandler() {

  }

  void echo(const std::string& s) {
    std::cerr << "Echo: " << s << std::endl;
  }
  
//:: name = "init"
//:: pd_name = pd_static_prefix + name
  void ${name}(){
    std::cerr << "In ${name}\n";
    ${pd_name}();
  }

//:: name = "cleanup"
//:: pd_name = pd_static_prefix + name
  void ${name}(){
    std::cerr << "In ${name}\n";
    ${pd_name}();
  }

//:: name = "client_init"
//:: pd_name = pd_static_prefix + name
  SessionHandle_t ${name}(const int32_t max_txn_size){
    std::cerr << "In ${name}\n";
    
    p4_pd_sess_hdl_t sess_hdl;
    ${pd_name}(&sess_hdl, max_txn_size);
    return sess_hdl;
  }

//:: name = "client_cleanup"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl){
    std::cerr << "In ${name}\n";
    
    return ${pd_name}(sess_hdl);
  }
  
//:: name = "begin_txn"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const bool isAtomic, const bool isHighPri){
    std::cerr << "In ${name}\n";
    
    return ${pd_name}(sess_hdl, isAtomic, isHighPri);
  }

//:: name = "verify_txn"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl){
    std::cerr << "In ${name}\n";
    
    return ${pd_name}(sess_hdl);
  }

//:: name = "abort_txn"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl){
    std::cerr << "In ${name}\n";
    
    return ${pd_name}(sess_hdl);
  }

//:: name = "commit_txn"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const bool hwSynchronous){
    std::cerr << "In ${name}\n";
    
    bool sendRsp;
    // TODO: sendRsp discarded ...
    return ${pd_name}(sess_hdl, hwSynchronous, &sendRsp);
  }

//:: name = "complete_operations"
//:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl){
    std::cerr << "In ${name}\n";
    
    return 0;
  }

  // control logging

//:: name = "rmt_log_level_set"
  void ${name}(P4LogLevel_t::type log_level) {
      std::cerr << "In ${name}\n";
      ::${name}((p4_log_level_t) log_level);
  }

//:: name = "rmt_log_level_get"
  P4LogLevel_t::type ${name}() {
      std::cerr << "In ${name}\n";
      return (P4LogLevel_t::type) ::${name}();
  }

  //:: name = "recirculation_enable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
  //:: name = "recirculation_disable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
  
  //:: name = "pktgen_enable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
  //:: name = "pktgen_disable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }

  //:: name = "pktgen_enable_recirc_pattern_matching"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
  //:: name = "pktgen_disable_recirc_pattern_matching"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
  //:: name = "pktgen_clear_port_down"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const int32_t dev, const int32_t port){
    std::cerr << "In ${name}\n";
    return ${pd_name}(sess_hdl, dev, port);
    
  }
 
  
  //:: name = "pktgen_cfg_app"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id, const PktGenAppCfg_t& cfg){
    std::cerr << "In ${name}\n";
    
    struct p4_pd_pktgen_app_cfg x;
    x.trigger_type = (p4_pd_pktgen_trigger_type_e)cfg.trigger_type;
    x.batch_count = cfg.batch_count;
    x.packets_per_batch = cfg.pkt_count;
    x.pattern_value = cfg.pattern_key;
    x.pattern_mask = cfg.pattern_msk;
    x.timer_nanosec = cfg.timer;
    x.ibg = cfg.ibg;
    x.ibg_jitter = cfg.ibg_jitter;
    x.ipg = cfg.ipg;
    x.ipg_jitter = cfg.ipg_jitter;
    x.source_port = cfg.src_port;
    x.increment_source_port = cfg.src_port_inc;
    x.pkt_buffer_offset = cfg.buffer_offset;
    x.length = cfg.length;
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id, &x);
    
  }
  
  //:: name = "pktgen_app_enable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id);
    
  }

  //:: name = "pktgen_app_disable"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id);
  }
  
  //:: name = "pktgen_write_pkt_buffer"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t offset, const int32_t size, const std::string& buf){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, offset, size, (uint8_t *)buf.c_str());
  }
  
  //:: name = "pktgen_get_batch_counter"
  //:: pd_name = pd_static_prefix + name
  int64_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id);
  }
  //:: name = "pktgen_get_pkt_counter"
  //:: pd_name = pd_static_prefix + name
  int64_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id);
  }

  //:: name = "pktgen_get_trigger_counter"
  //:: pd_name = pd_static_prefix + name
  int64_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t app_id){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, d, app_id);
  }

  //:: name = "set_meter_time"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const SessionHandle_t sess_hdl, const DevTarget_t& dev_tgt, const int32_t meter_time_disable){
    std::cerr << "In ${name}\n";
    p4_pd_dev_target_t d = {dev_tgt.dev_id, (uint16_t)dev_tgt.dev_pipe_id};
    return ${pd_name}(sess_hdl, meter_time_disable);
  }
};
