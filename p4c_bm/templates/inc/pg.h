/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _RMT_PG_H
#define _RMT_PG_H
#include "pd_static.h"

typedef enum pd_p4_pktgen_trigger_type {
  PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT=0,
  PD_PKTGEN_TRIGGER_TIMER_PERIODIC=1,
  PD_PKTGEN_TRIGGER_PORT_DOWN=2,
  PD_PKTGEN_TRIGGER_RECIRC_PATTERN=3
} p4_pd_pktgen_trigger_type_e;

typedef struct p4_pd_pktgen_app_cfg {
  p4_pd_pktgen_trigger_type_e trigger_type;
  uint16_t batch_count;
  uint16_t packets_per_batch;
  uint32_t pattern_value;
  uint32_t pattern_mask;
  uint32_t timer_nanosec;
  uint32_t ibg;
  uint32_t ibg_jitter;
  uint32_t ipg;
  uint32_t ipg_jitter;
  uint16_t source_port;
  bool increment_source_port;
  uint16_t pkt_buffer_offset;
  uint16_t length;
} p4_pd_pktgen_app_cfg;

enum {
  PKTGEN_SUCCESS = 0,
  PKTGEN_CONFIG_ERR
};

p4_pd_status_t
p4_pd_recirculation_enable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_recirculation_disable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_enable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_disable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_enable_recirc_pattern_matching(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_disable_recirc_pattern_matching(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_clear_port_down(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port);

p4_pd_status_t
p4_pd_pktgen_cfg_app(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id, p4_pd_pktgen_app_cfg* cfg);

p4_pd_status_t
p4_pd_pktgen_app_enable(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id);

p4_pd_status_t
p4_pd_pktgen_app_disable(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id);

p4_pd_status_t
p4_pd_pktgen_write_pkt_buffer(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t offset, uint32_t size, uint8_t* buf);

p4_pd_stat_t
p4_pd_pktgen_get_batch_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id);

p4_pd_stat_t
p4_pd_pktgen_get_pkt_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id);

p4_pd_stat_t
p4_pd_pktgen_get_trigger_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id);

#endif /* _RMT_PG_H */
