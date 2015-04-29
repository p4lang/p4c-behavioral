include "res.thrift"

namespace py mc_pd_rpc
namespace cpp mc_pd_rpc
namespace c_glib mc_pd_rpc

typedef i32 McHandle_t

service mc {
    # Multicast APIs.
//:: if enable_pre:
    McHandle_t mc_mgrp_create(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 mgid);
    i16 mc_mgrp_destroy(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3:i32 mgid);
    McHandle_t mc_l1_node_create(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i16 rid);
    i16 mc_l1_node_destroy(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 l1_hdl);
    i16 mc_l1_associate_node(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 mgrp_hdl, 4: i32 l1_hdl);
    McHandle_t mc_l2_node_create(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 l1_hdl, 4: list<byte> port_map);
    i16 mc_l2_node_destroy(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 l2_hdl);
    i16 mc_l2_node_update(1: res.SessionHandle_t sess_hdl, 2: res.DevTarget_t dev_tgt, 3: i32 l2_hdl, 4: list<byte> port_map);
//:: #endif
}
