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

#include "pg_int.h"
#include <p4_sim/pg.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <p4_sim/rmt.h>
#include "rmt_internal.h"
#include <time.h>

// Redefine MAX_PIPES

#undef MAX_PIPES
#define MAX_PIPES 4
#define PKTGEN_PORT 17
#define MAX_CHANNELS 4


pg_gbl_cfg* g_pg_gbl_cfg = NULL;
pg_thread_t* g_pg_thread = NULL;

static void nsleep(uint32_t nsec) {
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = nsec;
  nanosleep(&ts, NULL);
}


static void reset_vals(pg_gen_t* status) {
  status->curr_batch_clk = 0;
  status->curr_pkt = 0;
  status->curr_pkt_clk = 0;
  status->jitter_pkt_clks = 0;
  status->jitter_batch_clks = 0;
  status->cfg_pkt_jitter = 0;
  status->cfg_batch_jitter = 0;
  status->cum_pkt_jitter = 0;
}

/*
 * Generate packets
 * Ingress pipeline allocates memory to copy
 * this packet over, so we can re-use this memory.
 */

static void generate_packet(
    uint8_t* buffer,
    uint8_t pipe_id,
    uint8_t app_id,
    uint32_t trigger,
    p4_pd_pktgen_app_cfg* cfg,
    pg_gen_t* status) {

  uint8_t p_header[PGEN_HEADER_SZ] = {};
  uint16_t val = 0;
  uint16_t port_num = cfg->source_port;

  p_header[5] = (status->curr_pkt) & 0xFF;
  p_header[4] = (status->curr_pkt >> 8) & 0xFF;

  switch(cfg->trigger_type) {
    case PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT:
    case PD_PKTGEN_TRIGGER_TIMER_PERIODIC:
      p_header[3] = (status->curr_batch) & 0xFF;
      p_header[2] = (status->curr_batch >> 8) & 0xFF;
      p_header[1] = 0;
      break;
    case PD_PKTGEN_TRIGGER_PORT_DOWN:
      p_header[3] = trigger & 0xFF;
      p_header[2] = (trigger >> 8) & 0xFF;
      p_header[1] = 0;
      break;
    case PD_PKTGEN_TRIGGER_RECIRC_PATTERN:
      val = trigger & 0xFFFF;
      val += status->curr_batch;
      p_header[3] = val & 0xFF;
      p_header[2] = (val >> 8) & 0xFF;
      p_header[1] = (trigger >> 16) & 0xFF;
      break;
  }
  p_header[0] = ((app_id << 2)  | (pipe_id & 0x3));

  memcpy(buffer, p_header, sizeof(p_header)/sizeof(uint8_t));
  if (cfg->increment_source_port) {
    port_num += status->curr_pkt;
  }
  if (port_num >= MAX_PKTGEN_PORT) {
    port_num -= MAX_PKTGEN_PORT;
  }
  RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:GENERATING:Pipe:[%u]:App[%u]:Type:[%u]:batch[%u]/[%u]:pkt:[%u]/[%u]:ingress_port:[%u]\n",
      pipe_id, app_id, cfg->trigger_type, status->curr_batch, cfg->batch_count, status->curr_pkt, cfg->packets_per_batch, port_num);

  rmt_process_pkt(port_num, buffer, cfg->length + PGEN_HEADER_SZ);
}

static void ibg_sleep(p4_pd_pktgen_app_cfg* cfg,
    pg_gen_t* status) {
  uint32_t inter_batch_time = 0;
  uint32_t total_pkt_time = 0;
  inter_batch_time = cfg->ibg + status->cfg_batch_jitter;
  total_pkt_time = (cfg->ipg * cfg->packets_per_batch) + status->cum_pkt_jitter;
  if (inter_batch_time > total_pkt_time) {
    nsleep(inter_batch_time - total_pkt_time);
  }
}

static void new_pkt(uint8_t* buffer,
    uint8_t pipe,
    uint8_t app_id,
    uint32_t trigger_id,
    p4_pd_pktgen_app_cfg* cfg,
    pg_gen_t* status,
    pg_counter_t* counters) {
  /*
   * New packet here
   */

  generate_packet(buffer, pipe, app_id, trigger_id, cfg, status);
  counters->pkt_cnt++;
  status->curr_pkt ++;
  if (status->curr_pkt < cfg->packets_per_batch) {
    nsleep(cfg->ipg + status->cfg_pkt_jitter);
    status->curr_pkt_clk = 0;
    status->jitter_pkt_clks = 0;
  } else {
    if (status->curr_batch_clk == CLK_DONE) {
      status->curr_batch ++;
      counters->batch_cnt++;
      if (status->curr_batch < cfg->batch_count) {
        ibg_sleep(cfg, status);
        reset_vals(status);
      } else {
        status->curr_pkt_clk = CLK_DONE;
        status->jitter_pkt_clks = 0;
      }
    } else {
      status->curr_pkt_clk = CLK_DONE;
      status->jitter_pkt_clks = 0;
    }
  }

}




static void new_batch(p4_pd_pktgen_app_cfg* cfg,
    pg_gen_t* status,
    pg_counter_t* counters) {

  if (status->curr_pkt_clk == CLK_DONE) {
    status->curr_batch ++;
    counters->batch_cnt++;
    if(status->curr_batch < cfg->batch_count) {
      ibg_sleep(cfg, status);
      reset_vals(status);
    } else {
      status->curr_batch_clk = CLK_DONE;
    }
  } else {
    status->curr_batch_clk = CLK_DONE;
  }
}

static bool exit_pg(void) {

  bool run = false;
  int i = 0;
  for (i = 0; i < MAX_PIPES; i ++) {
    run |= g_pg_gbl_cfg[i].pgen_en;
  }
  return !run;

}


static bool gen_packets(int pipe,
    uint8_t* buffer,
    uint8_t app_id,
    uint32_t trigger_id,
    p4_pd_pktgen_app_cfg* app_cfg,
    pg_gen_t* status,
    pg_counter_t* counters) {

  if (status->curr_pkt_clk == 0 && status->curr_batch_clk == 0) {
    counters->trigger_cnt ++;
    memcpy(buffer + PGEN_HEADER_SZ, g_pg_gbl_cfg[pipe].pgen_buf + app_cfg->pkt_buffer_offset, app_cfg->length);
  }
  if (status->curr_pkt_clk == CLK_DONE && status->curr_batch_clk == CLK_DONE) {
    status->curr_batch = 0;
    status->curr_pkt = 0;
    status->cum_pkt_jitter = 0; 
    return false;
  }
#if 0
  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
      "PktGen:App:[%u]:c_pkt_clk:%d:c_batch_clk:%d:c_p_jitter:%d:c_b_jitter:%d:ipg:%u:ibg:%u:max_ipg_jitter:%u:max_ibg_jitter:%u:cfg_ipg_jitter:%u:cfg_ibg_jitter:%u\n",
      app_id,
      status->curr_pkt_clk,
      status->curr_batch_clk,
      status->jitter_pkt_clks,
      status->jitter_batch_clks,
      app_cfg->ipg,
      app_cfg->ibg,
      app_cfg->ipg_jitter,
      app_cfg->ibg_jitter,
      status->cfg_pkt_jitter,
      status->cfg_batch_jitter);
#endif

  if (status->curr_pkt_clk != CLK_DONE) {
    if(status->curr_pkt_clk < app_cfg->ipg) {
      status->curr_pkt_clk ++;
    } else {
      if (app_cfg->ipg_jitter) {
        if (status->jitter_pkt_clks == 0) {
          status->cfg_pkt_jitter = rand()%app_cfg->ipg_jitter;
        }
        if (status->jitter_pkt_clks < status->cfg_pkt_jitter) {
          status->jitter_pkt_clks ++;
        } else {
          status->cum_pkt_jitter += status->cfg_pkt_jitter;
          new_pkt(buffer, pipe, app_id, trigger_id, app_cfg, status, counters);
        }
      } else {
        new_pkt(buffer, pipe, app_id, trigger_id, app_cfg, status, counters);
      }
    }
  }

  if (status->curr_batch_clk != CLK_DONE) {
    if (status->curr_batch_clk < app_cfg->ibg) {
      status->curr_batch_clk ++;
    } else {
      if (status->curr_pkt_clk == CLK_DONE) {
        if (app_cfg->ibg_jitter) {
          if (status->jitter_batch_clks == 0) {
            status->cfg_batch_jitter = rand()%app_cfg->ibg_jitter;
          }
          if (status->jitter_batch_clks < status->cfg_batch_jitter) {
            status->jitter_batch_clks++;
          } else {
            new_batch(app_cfg, status, counters);
          }

        } else {
          new_batch(app_cfg, status, counters);
        }

      } else {
        new_batch(app_cfg, status, counters);
      }
    }

  }
  return true;

}

static void delete_triggers(tommy_list* tr_list) {
  tommy_node* tr_node = tommy_list_head(tr_list);
  while (tr_node != NULL) {
    pg_trigger_t* m_trigger = tr_node->data;
    if (m_trigger->st_gen) {
      free(m_trigger->st_gen);
      m_trigger->st_gen = NULL;
    }
    free(m_trigger);
    m_trigger = NULL;
    tr_node = tr_node->next;
  }

}
static uint32_t get_u32(uint8_t* pkt, int len, int nbytes) {
  uint32_t rval = 0;
  int i = 0;
  if (nbytes > len) nbytes = len;
  while (i < nbytes) {
    rval <<= 8;
    rval |= pkt[i++];
  }
  return rval;

}

static void setup_trigger(tommy_list* tr_list, uint32_t key) {

  tommy_node* trigger_node_obj = tommy_list_head(tr_list);
  bool found = false;
  pg_trigger_t* m_node = NULL;
  while(trigger_node_obj != NULL) {
    m_node = trigger_node_obj->data;
    if (m_node->trigger_id == key) {
      m_node->n_count ++;
      m_node->is_done = false;
      found = true;
      break;
    }
    trigger_node_obj = trigger_node_obj->next;
  }
  if (!found) {
    m_node = calloc(1, sizeof(pg_trigger_t));
    m_node->n_count = 1;
    m_node->is_done = false;
    m_node->trigger_id = key;
    tommy_list_insert_tail(tr_list,
        &(m_node->trigger_node),
        m_node);
  }
}



void pktgen_snoop_recirc(int pipe, int port, uint8_t* pkt, uint32_t len) {


  pthread_mutex_lock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen: Rx Recirc:Dev[%u]:Port:[%u]:Len:[%u]\n", pipe, port, len);

  if (!(g_pg_gbl_cfg[pipe].g_port_cfg)) goto exit_snoop;
  if (!g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_snoop_en) goto exit_snoop;

  tommy_hashlin_node* t_node = tommy_hashlin_bucket(&(g_pg_gbl_cfg[pipe].g_type_hash),
      tommy_inthash_u32(PD_PKTGEN_TRIGGER_RECIRC_PATTERN));
  uint32_t val = get_u32(pkt, len, 4);
  uint32_t cmp_val;
  while (t_node) {
    pg_app_t* app_node = t_node->data;
    t_node = t_node->next;
    if (!app_node->is_en) continue;
    cmp_val = (val | app_node->cfg->pattern_mask);
    if (cmp_val != (app_node->cfg->pattern_value | app_node->cfg->pattern_mask)) {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:Recirc:Pipe[%u]:Port:[%u]:Len:[%u]: NOT_MATCHED\n", pipe, port, len);
      continue;
    }
    RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:Recirc:Pipe[%u]:Port:[%u]:Len:[%u]:MATCHED\n", pipe, port, len);
    setup_trigger(&(app_node->status->trigger_lst), val);
    app_node->status->is_init = true;
  }

exit_snoop:
  pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
}



static void process_apps(int pipe) {

  p4_pd_pktgen_trigger_type_e ttype = PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT;
  while (ttype <= PD_PKTGEN_TRIGGER_RECIRC_PATTERN) {
    tommy_hashlin_node* t_node = tommy_hashlin_bucket(&(g_pg_gbl_cfg[pipe].g_type_hash),
        tommy_inthash_u32(ttype));
    while(t_node) {
      pthread_mutex_lock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
      pg_app_t* app_node = t_node->data;
      t_node = t_node->next;
      if (!app_node->is_en) {
        pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
        continue;
      }
      if (!app_node->status->is_init && ((ttype == PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT) 
          || (ttype == PD_PKTGEN_TRIGGER_TIMER_PERIODIC))) {
        if (!tommy_list_empty(&(app_node->status->trigger_lst))) {
          delete_triggers(&(app_node->status->trigger_lst));
        }

        pg_trigger_t* tr_node = calloc(1, sizeof(pg_trigger_t));
        tr_node->is_done = false;
        if (ttype == PD_PKTGEN_TRIGGER_TIMER_ONE_SHOT) {
          tr_node->n_count = 1;
          //nsleep(app_node->cfg->timer_nanosec);
        }
        tommy_list_insert_tail(&(app_node->status->trigger_lst),
            &(tr_node->trigger_node),
            tr_node);
        app_node->status->is_init = true;
        RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:App[%u]:Type[%u]:ARMED\n", app_node->app_id, ttype);
      }
      tommy_node* trigger_node_obj = tommy_list_head(&(app_node->status->trigger_lst));
      tommy_node* tmp_node;
      while (trigger_node_obj != NULL) {
        pg_trigger_t* m_node = trigger_node_obj->data;
        tmp_node = trigger_node_obj;
        trigger_node_obj = trigger_node_obj->next;
        if (!m_node->st_gen) {
          RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:App[%u]:Type:[%u]:Triggered\n", app_node->app_id, ttype);
          m_node->st_gen = calloc(1, sizeof(pg_gen_t));
        }
        if (!gen_packets(pipe,
              app_node->pkt_buffer,
              app_node->app_id,
              m_node->trigger_id,
              app_node->cfg,
              m_node->st_gen,
              app_node->counters)) {
          if (ttype != PD_PKTGEN_TRIGGER_TIMER_PERIODIC) {
            m_node->n_count --;
            if (m_node->n_count == 0) m_node->is_done = true;
          } else {
            nsleep(app_node->cfg->timer_nanosec);
            m_node->st_gen->curr_batch_clk = 0;
            m_node->st_gen->curr_pkt_clk = 0;
            reset_vals(m_node->st_gen);
          }
             RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:App:[%u]:Trigger ID:[%u]:Triggers Left:[%u]\n",
             app_node->app_id,
             m_node->trigger_id,
             m_node->n_count);

        }
        if (m_node->is_done) {
          RMT_LOG(P4_LOG_LEVEL_VERBOSE,"PktGen:App:[%u]:Type:[%u]:TriggerID:[%u]:Done\n",
              app_node->app_id,
              ttype,
              m_node->trigger_id);
          free(m_node->st_gen);
          m_node->st_gen = NULL;
          tommy_list_remove_existing(&(app_node->status->trigger_lst),
              tmp_node);
          free(m_node);
          m_node = NULL;
        }
      }
      pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
    }
    ttype++;
  }
}

static void port_down_trigger(int pipe, int port) {

  tommy_hashlin_node* t_node = tommy_hashlin_bucket(&(g_pg_gbl_cfg[pipe].g_type_hash),
      tommy_inthash_u32(PD_PKTGEN_TRIGGER_PORT_DOWN));
  while (t_node) {
    pg_app_t* app_node = t_node->data;
    t_node = t_node->next;
    if (!app_node->is_en) continue;
    setup_trigger(&(app_node->status->trigger_lst), port);
    app_node->status->is_init = true;
    RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:Pipev[%u]:Port[%u]:App[%u]:DOWN Trigger added\n",
        pipe, port, app_node->app_id);
  }

}

/*
 * Main PktGen processing loop
 */

static void* pktgen_thread(void* arg) {
  int i = 0;
  while (!exit_pg()) {
    if (g_pg_gbl_cfg[i].pgen_en) {
      process_apps(i);
    }
    i++;
    i%=MAX_PIPES;
  }
  return NULL;
}

void pktgen_start(int pipe) {

  g_pg_gbl_cfg[pipe].pgen_en = true;
  if (!g_pg_thread) {
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen: Thread Started\n");
    g_pg_thread = (pg_thread_t *)calloc(1, sizeof(pg_thread_t));
    pthread_create(&(g_pg_thread->thread_id),
        NULL,
        &pktgen_thread,
        NULL);
  }
  //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:START:Pipe[%u]\n", pipe);
}

void pktgen_stop(int pipe) {
  if (!g_pg_gbl_cfg[pipe].pgen_en) return;
  g_pg_gbl_cfg[pipe].pgen_en = false;
  //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:STOP:Pipe[%u]\n", pipe);
  if (exit_pg()) {
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen: Thread Stopped\n");
    pthread_join(g_pg_thread->thread_id, NULL);
    free(g_pg_thread);
    g_pg_thread = NULL;

  }

}

void pktgen_alloc_port_cfg(int pipe) {
  assert(pipe >= 0 && pipe < MAX_PIPES);
  pthread_mutex_lock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
  if (!g_pg_gbl_cfg[pipe].g_port_cfg) {
    g_pg_gbl_cfg[pipe].g_port_cfg = (pg_port_cfg *)calloc(PORT_COUNT_PER_PIPE_MAX, sizeof(pg_port_cfg));
  }
  pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
}

bool pktgen_is_recirc_en(int pipe, int port) {
  if (!g_pg_gbl_cfg[pipe].g_port_cfg) return false;
  return g_pg_gbl_cfg[pipe].g_port_cfg[port].recir_en;

}


static int pg_appid_cmp(const void* arg, const void* obj) {
  return *(const uint32_t *)arg != ((const pg_app_t *)obj)->app_id;
}
static int pg_type_cmp(const void* arg, const void* obj) {
  return *(const p4_pd_pktgen_trigger_type_e *)arg != ((const pg_app_t *)obj)->cfg->trigger_type;
}

void pktgen_port_flap(int port, bool status) {
  int pipe = port >> LOCAL_PORT_BITS;
  port &= LOCAL_PORT_MASK;

  assert(g_pg_gbl_cfg != NULL);
  assert(pipe >= 0 && pipe < MAX_PIPES);
  pktgen_alloc_port_cfg(pipe);
  pthread_mutex_lock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
  if (!status && !(g_pg_gbl_cfg[pipe].g_port_cfg[port].port_down)){
    g_pg_gbl_cfg[pipe].g_port_cfg[port].port_down = true;
    RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:Port[%u] DOWN:HANDLING\n", port);
    port_down_trigger(pipe, port);
  } else if (!status) {
    RMT_LOG(P4_LOG_LEVEL_VERBOSE, "PktGen:Port[%u] DOWN:NOT HANDLING\n", port);
  }
  pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe].cfg_mutex));
}

void reset_buffer(int pipe_id, pg_app_t* m_app, p4_pd_pktgen_app_cfg* cfg) {
  if (m_app->pkt_buffer != NULL) {
    free(m_app->pkt_buffer);
    m_app->pkt_buffer = NULL;
  }
  m_app->pkt_buffer = (uint8_t *)calloc(PGEN_HEADER_SZ + cfg->length, sizeof(uint8_t));
}


p4_pd_status_t pktgen_app_cfg(int pipe_id, uint32_t app_id, p4_pd_pktgen_app_cfg* cfg) {
  assert(cfg != NULL);
  assert(pipe_id >= 0 && pipe_id < MAX_PIPES);
  //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:CONFIGURING:Pipe[%u]:App[%u]\n", pipe_id, app_id);
  pthread_mutex_lock(&(g_pg_gbl_cfg[pipe_id].cfg_mutex));
  pg_app_t* m_app = tommy_hashlin_search(&(g_pg_gbl_cfg[pipe_id].g_appid_hash),
      pg_appid_cmp, &app_id, tommy_inthash_u32(app_id));
  if (!m_app) {
    m_app = (pg_app_t *)calloc(1, sizeof(pg_app_t));
    m_app->is_en = false;
    m_app->counters = (pg_counter_t *)calloc(1, sizeof(pg_counter_t));
    m_app->status = (pg_app_status_t *)calloc(1, sizeof(pg_app_status_t));
    m_app->status->is_init = false;
    tommy_list_init(&(m_app->status->trigger_lst));
    m_app->cfg = (p4_pd_pktgen_app_cfg *)calloc(1, sizeof(p4_pd_pktgen_app_cfg));
    memcpy(m_app->cfg, cfg, sizeof(p4_pd_pktgen_app_cfg));
    m_app->app_id = app_id;
    m_app->pkt_buffer = NULL;
    tommy_hashlin_insert(&(g_pg_gbl_cfg[pipe_id].g_appid_hash),
        &m_app->app_hash_node,
        m_app,
        tommy_inthash_u32(m_app->app_id));
    tommy_hashlin_insert(&(g_pg_gbl_cfg[pipe_id].g_type_hash),
        &m_app->type_hash_node,
        m_app,
        tommy_inthash_u32(m_app->cfg->trigger_type));
    tommy_list_insert_tail(&(g_pg_gbl_cfg[pipe_id].g_app_lst),
        &m_app->app_lst_node,
        m_app);

  } else {
    if (m_app->cfg->trigger_type != cfg->trigger_type) {
      assert(m_app->is_en == false);
      delete_triggers(&(m_app->status->trigger_lst));
      tommy_hashlin_remove_existing(&(g_pg_gbl_cfg[pipe_id].g_type_hash),
          &m_app->type_hash_node);
      memcpy(m_app->cfg, cfg, sizeof(p4_pd_pktgen_app_cfg));

      tommy_hashlin_insert(&(g_pg_gbl_cfg[pipe_id].g_type_hash),
          &m_app->type_hash_node,
          m_app,
          tommy_inthash_u32(m_app->cfg->trigger_type));
      m_app->status->is_init = false;
    } else {
      memcpy(m_app->cfg, cfg, sizeof(p4_pd_pktgen_app_cfg));
    }
  }
  reset_buffer(pipe_id, m_app, cfg);
  //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:CONFIGURED:Pipe[%u]:App[%u]\n", pipe_id, app_id);
  pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe_id].cfg_mutex));

  return PKTGEN_SUCCESS;
}
p4_pd_status_t pktgen_app_arm(int pipe_id, uint32_t app_id, bool en) {

  assert(pipe_id >= 0 && pipe_id < MAX_PIPES);

  pg_app_t* m_app = tommy_hashlin_search(&(g_pg_gbl_cfg[pipe_id].g_appid_hash),
      pg_appid_cmp, &app_id, tommy_inthash_u32(app_id));
  if (m_app == NULL) {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:EN_FAILURE:NOT_CONFIGURED:Pipe[%u]:App[%u]\n", pipe_id, app_id);
    goto app_arm_exit;

  }
  //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:Pipe:[%u]:App[%u]:EN[%d]\n", pipe_id, app_id, en);
  m_app->is_en = en;
  if (!m_app->is_en) {
    m_app->status->is_init = false;
  }
app_arm_exit:
  return PKTGEN_SUCCESS;
}
pg_counter_t* pktgen_get_stats(int pipe_id, uint32_t app_id) {

  assert(pipe_id >= 0 && pipe_id < MAX_PIPES);
  pthread_mutex_lock(&(g_pg_gbl_cfg[pipe_id].cfg_mutex));
  pg_app_t* m_app = tommy_hashlin_search(&(g_pg_gbl_cfg[pipe_id].g_appid_hash),
      pg_appid_cmp, &app_id, tommy_inthash_u32(app_id));
  pthread_mutex_unlock(&(g_pg_gbl_cfg[pipe_id].cfg_mutex));
  if (m_app != NULL) {
    return m_app->counters;
  }
  return NULL;
}

void pktgen_init(void) {
  int i = 0;
  g_pg_gbl_cfg = (pg_gbl_cfg *)calloc(MAX_PIPES, sizeof(pg_gbl_cfg));
  for (i = 0; i < MAX_PIPES; i ++) {
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:INIT:PIPE[%u]\n", i);
    g_pg_gbl_cfg[i].g_port_cfg = NULL;
    g_pg_gbl_cfg[i].pgen_en = false;
    g_pg_gbl_cfg[i].pgen_buf = (uint8_t *)calloc(MAX_PGEN_BUFFER_SZ, sizeof(uint8_t));
    tommy_hashlin_init(&(g_pg_gbl_cfg[i].g_type_hash));
    tommy_hashlin_init(&(g_pg_gbl_cfg[i].g_appid_hash));
    tommy_list_init(&(g_pg_gbl_cfg[i].g_app_lst));
    pthread_mutex_init(&(g_pg_gbl_cfg[i].cfg_mutex), NULL);
  }
  portmgr_register_cb(pktgen_port_flap);
}

void pktgen_cleanup(void) {

  int i = 0;
  for (i = 0; i < MAX_PIPES; i ++) {
    //RMT_LOG(P4_LOG_LEVEL_TRACE, "PktGen:CLEANUP:PIPE[%u]\n", i);
    pthread_mutex_lock(&(g_pg_gbl_cfg[i].cfg_mutex));
    if (g_pg_gbl_cfg[i].g_port_cfg) {
      free(g_pg_gbl_cfg[i].g_port_cfg);
      g_pg_gbl_cfg[i].g_port_cfg = NULL;
    }
    if (g_pg_gbl_cfg[i].pgen_buf) {
      free(g_pg_gbl_cfg[i].pgen_buf);
      g_pg_gbl_cfg[i].pgen_buf = NULL;
    }
    tommy_node* app_node_obj = tommy_list_head(&(g_pg_gbl_cfg[i].g_app_lst));
    while (app_node_obj != NULL) {
      pg_app_t* app_obj = app_node_obj->data;
      free(app_obj->pkt_buffer);
      app_obj->pkt_buffer = NULL;
      free(app_obj->counters);
      app_obj->counters = NULL;
      free(app_obj->cfg);
      app_obj->cfg = NULL;
      tommy_node* trigger_node_obj = tommy_list_head(&(app_obj->status->trigger_lst));
      while (trigger_node_obj != NULL) {
        pg_trigger_t* trigger_obj = trigger_node_obj->data;
        if (trigger_obj->st_gen != NULL) {
          free(trigger_obj->st_gen);
          trigger_obj->st_gen = NULL;
        }
        free(trigger_obj);
        trigger_node_obj = trigger_node_obj->next;
      }
      free(app_obj->status);
      app_obj->status = NULL;
      free(app_obj);
      app_node_obj = app_node_obj->next;
    }
    tommy_hashlin_done(&(g_pg_gbl_cfg[i].g_type_hash));
    tommy_hashlin_done(&(g_pg_gbl_cfg[i].g_appid_hash));
    pthread_mutex_unlock(&(g_pg_gbl_cfg[i].cfg_mutex));
    pthread_mutex_destroy(&(g_pg_gbl_cfg[i].cfg_mutex));

  }
  free(g_pg_gbl_cfg);
  g_pg_gbl_cfg = NULL;
}

uint16_t pktgen_get_pipe(uint32_t port) { return port >> LOCAL_PORT_BITS; }
uint16_t pktgen_get_port(uint32_t port) { return port & LOCAL_PORT_MASK; }
