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

#include <p4_sim/pd_static.h>
#include <p4_sim/pg.h>
#include <p4_sim/rmt.h>
#include "rmt_internal.h"
#include "portmanager.h"
#include "pg_int.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>


p4_pd_status_t 
p4_pd_recirculation_enable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  port &= LOCAL_PORT_MASK;
  pktgen_alloc_port_cfg(pipe);
  g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_en = true;
  return PKTGEN_SUCCESS;

}
p4_pd_status_t 
p4_pd_recirculation_disable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  port &= LOCAL_PORT_MASK;
  pktgen_alloc_port_cfg(pipe);
  g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_en = false;
  return PKTGEN_SUCCESS;

}
p4_pd_status_t 
p4_pd_pktgen_enable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  pktgen_start(pipe);
  return PKTGEN_SUCCESS;
}
p4_pd_status_t 
p4_pd_pktgen_disable(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  pktgen_stop(pipe);
  return PKTGEN_SUCCESS;
}
p4_pd_status_t 
p4_pd_pktgen_enable_recirc_pattern_matching(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  port &= LOCAL_PORT_MASK;
  pktgen_alloc_port_cfg(pipe);
  g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_snoop_en = true;
  return PKTGEN_SUCCESS;
}

p4_pd_status_t 
p4_pd_pktgen_disable_recirc_pattern_matching(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  port &= LOCAL_PORT_MASK;
  pktgen_alloc_port_cfg(pipe);
  g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_snoop_en = false;
  return PKTGEN_SUCCESS;

}

p4_pd_status_t 
p4_pd_pktgen_clear_port_down(p4_pd_sess_hdl_t shdl, uint32_t dev, uint32_t port) {
  dev = 0;
  if ((dev >= MAX_CHIPS) || (port >= PORT_COUNT_PER_CHIP_MAX)) return PKTGEN_CONFIG_ERR;
  assert(g_pg_gbl_cfg != NULL);
  uint8_t pipe = (port >> LOCAL_PORT_BITS) & 0x3;
  port &= LOCAL_PORT_MASK;
  pktgen_alloc_port_cfg(pipe);
  g_pg_gbl_cfg[pipe].g_port_cfg[port].port_down = false;
  return 0;
}
p4_pd_status_t 
p4_pd_pktgen_cfg_app(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id, p4_pd_pktgen_app_cfg* cfg) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;

  // Batch count & Packet Count are zero based
  cfg->batch_count ++;
  cfg->packets_per_batch ++;

  // Pattern mask is inverted mask
  cfg->pattern_mask = ~(cfg->pattern_mask);

  return pktgen_app_cfg(dev.dev_pipe_id, app_id, cfg);
}
p4_pd_status_t 
p4_pd_pktgen_app_enable(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  return pktgen_app_arm(dev.dev_pipe_id, app_id, true);

}
p4_pd_status_t 
p4_pd_pktgen_app_disable(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  return pktgen_app_arm(dev.dev_pipe_id, app_id, false);
}

p4_pd_status_t 
p4_pd_pktgen_write_pkt_buffer(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t offset, uint32_t size, uint8_t* buf) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  if (!size || ((offset + size) >= MAX_PGEN_BUFFER_SZ)) return PKTGEN_CONFIG_ERR;
  if (!g_pg_gbl_cfg[dev.dev_pipe_id].pgen_buf) {
    g_pg_gbl_cfg[dev.dev_pipe_id].pgen_buf = (uint8_t *)calloc(MAX_PGEN_BUFFER_SZ, sizeof(uint8_t));
  }
  memcpy(&(g_pg_gbl_cfg[dev.dev_pipe_id].pgen_buf[offset]), buf, size);
  return PKTGEN_SUCCESS;
}

p4_pd_stat_t 
p4_pd_pktgen_get_batch_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  pg_counter_t* cnt = pktgen_get_stats(dev.dev_pipe_id, app_id);
  if (cnt != NULL) {
    return cnt->batch_cnt;
  } else return 0;

}
p4_pd_stat_t 
p4_pd_pktgen_get_pkt_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  pg_counter_t* cnt = pktgen_get_stats(dev.dev_pipe_id, app_id);
  if (cnt != NULL) {
    return cnt->pkt_cnt;
  } else return 0;
}
p4_pd_stat_t 
p4_pd_pktgen_get_trigger_counter(p4_pd_sess_hdl_t shdl, p4_pd_dev_target_t dev, uint32_t app_id) {
  dev.device_id = 0;
  if ((dev.device_id >= MAX_CHIPS)) return PKTGEN_CONFIG_ERR;
  pg_counter_t* cnt = pktgen_get_stats(dev.dev_pipe_id, app_id);
  if (cnt != NULL) {
    return cnt->trigger_cnt;
  } else return 0;
}

