
namespace py devport_mgr_pd_rpc
namespace cpp devport_mgr_pd_rpc
namespace c_glib devport_mgr_pd_rpc

service devport_mgr {
# Add port stuff here as well
  i32 devport_mgr_add_port (1: i32 dev_id, 2: i32 port_id,
      3: i32 port_width, 4: bool autoneg_enable,
      5: i32 port_speeds, 6: i32 port_fec_types,
      7: i32 tx_pause, 8: i32 rx_pause);
  i32 devport_mgr_remove_port (1: i32 dev_id, 2: i32 port_id);
}
