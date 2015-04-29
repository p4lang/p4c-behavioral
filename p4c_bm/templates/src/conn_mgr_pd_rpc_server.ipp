//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include "conn_mgr.h"

#include <iostream>

extern "C" {
#include <p4_sim/pd_static.h>
#include <p4_sim/rmt.h>
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

};
