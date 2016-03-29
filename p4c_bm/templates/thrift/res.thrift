namespace py res_pd_rpc
namespace cpp res_pd_rpc
namespace c_glib res_pd_rpc

typedef i32 SessionHandle_t
struct DevTarget_t {
1: required i32 dev_id;
2: required i16 dev_pipe_id;
}

enum P4LogLevel_t {
  P4_LOG_LEVEL_NONE = 0, # otherwise will it start from 1?
  P4_LOG_LEVEL_FATAL,
  P4_LOG_LEVEL_ERROR,
  P4_LOG_LEVEL_WARN,
  P4_LOG_LEVEL_INFO,
  P4_LOG_LEVEL_VERBOSE,
  P4_LOG_LEVEL_TRACE
}

enum PktGenTriggerType_t {
  TIMER_ONE_SHOT=0,
  TIMER_PERIODIC=1,
  PORT_DOWN=2,
  RECIRC_PATTERN=3
}

struct PktGenAppCfg_t {
1: required PktGenTriggerType_t trigger_type;
2: required i32 batch_count;
3: required i32 pkt_count;
4: required i32 pattern_key;
5: required i32 pattern_msk;
6: required i32 timer;
7: required i32 ibg;
8: required i32 ibg_jitter;
9: required i32 ipg;
10: required i32 ipg_jitter;
11: required i32 src_port;
12: required i32 src_port_inc;
13: required i32 buffer_offset;
14: required i32 length;
}
