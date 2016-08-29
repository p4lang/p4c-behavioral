include "res.thrift"

namespace py conn_mgr_pd_rpc
namespace cpp conn_mgr_pd_rpc
namespace c_glib conn_mgr_pd_rpc


service conn_mgr {
# Test echo interface
  void echo(1:string s);

  void init();

  void cleanup();

  res.SessionHandle_t client_init();

  i32 client_cleanup(1:res.SessionHandle_t sess_hdl);

  i32 begin_txn(1:res.SessionHandle_t sess_hdl, 2:bool isAtomic, 3:bool isHighPri);

  i32 verify_txn(1:res.SessionHandle_t sess_hdl);

  i32 abort_txn(1:res.SessionHandle_t sess_hdl);

  i32 commit_txn(1:res.SessionHandle_t sess_hdl, 2:bool hwSynchronous);

  i32 complete_operations(1:res.SessionHandle_t sess_hdl);

# control logging
  void rmt_log_level_set(1:res.P4LogLevel_t log_level);
  res.P4LogLevel_t rmt_log_level_get();

  i32 recirculation_enable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 recirculation_disable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 pktgen_enable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 pktgen_disable(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 pktgen_enable_recirc_pattern_matching(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 pktgen_disable_recirc_pattern_matching(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);
  i32 pktgen_clear_port_down(1:res.SessionHandle_t sess_hdl, 2:i32 dev, 3:i32 port);

  i32 pktgen_cfg_app(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id, 4:res.PktGenAppCfg_t cfg);

  i32 pktgen_app_enable(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id);
  i32 pktgen_app_disable(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id);
  i32 pktgen_write_pkt_buffer(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 offset, 4:i32 size, 5:binary buf);
  i64 pktgen_get_batch_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id);
  i64 pktgen_get_pkt_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id);
  i64 pktgen_get_trigger_counter(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 app_id);

  i32 set_meter_time(1:res.SessionHandle_t sess_hdl, 2:res.DevTarget_t dev_tgt, 3:i32 meter_time_disable);

}
