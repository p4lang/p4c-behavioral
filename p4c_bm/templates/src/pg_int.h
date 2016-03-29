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

#ifndef __PG_INT_H
#define __PG_INT_H

#include <p4_sim/pg.h>
#include <p4utils/tommyhashlin.h>
#include <p4utils/tommylist.h>
#include <signal.h>

#include "portmanager.h"
#define MAX_PGEN_BUFFER_SZ 16384
#define LOCAL_PORT_BITS 7
#define LOCAL_PORT_MASK 0x7F
#define PORT_BITS 9
#define PORT_MASK 0x1FF
#define CACHE_BLK_SIZE 64
#define PGEN_HEADER_SZ 6
#define PORT_COUNT_PER_PIPE_MAX 128

/*
 * Specifies how many "real" usecs does
 * every delay element represent
 */

#define CLK_TIME 300
#define CLK_DONE -1
//#define MAX_PKTGEN_PORT 287
#define MAX_PKTGEN_PORT 512


typedef struct pg_port_cfg {
  bool recir_en;
  bool recir_snoop_en;
  bool port_down;
  uint8_t down_cnt;
} pg_port_cfg;



typedef struct pg_counter_t {
  p4_pd_stat_t batch_cnt;
  p4_pd_stat_t pkt_cnt;
  p4_pd_stat_t trigger_cnt;

} pg_counter_t;

/*
 * This struct is used @ generation time
 * For each round of generation, so per-trigger
 */

typedef struct pg_gen_t {
  int32_t curr_pkt_clk;
  int32_t curr_batch_clk;
  uint16_t curr_pkt;
  uint16_t curr_batch;
  uint32_t cfg_pkt_jitter;
  uint32_t cum_pkt_jitter;
  uint32_t jitter_pkt_clks;
  uint32_t cfg_batch_jitter;
  uint32_t jitter_batch_clks;
} pg_gen_t;

/*
 * Defines what caused the trigger
 * The triggering event is responsible for populating
 * this. The generator is responsible for parsing this
 * and generating the appropriate packets. A linked list of
 * these events is maintained.
 */


typedef struct pg_trigger_t {
  tommy_node trigger_node;
  pg_gen_t* st_gen;
  /* Port number, recirculation header */
  uint32_t trigger_id;
  sig_atomic_t n_count;
  bool is_done;
}pg_trigger_t;


typedef struct pg_app_status_t {
  bool is_init;
  tommy_list trigger_lst;
} pg_app_status_t;


typedef struct pg_app_t {
  tommy_node type_hash_node;
  tommy_node app_hash_node;
  tommy_node app_lst_node;
  uint8_t app_id;
  sig_atomic_t is_en;
  pg_app_status_t* status;
  pg_counter_t* counters;
  p4_pd_pktgen_app_cfg* cfg;
  uint8_t* pkt_buffer;

} pg_app_t;

/*
 * Thread for pktgen, 1 thread/device
 * Passed as argument to each thread
 */

typedef struct pg_thread_t {
  pthread_t thread_id;
  int pipe_id;
  /* Add other stuff here */
}pg_thread_t;


/*
 * Per device gbl_cfg
 */

typedef struct __attribute__((__aligned__(CACHE_BLK_SIZE))) {
  bool pgen_en;
  uint8_t *pgen_buf;
  pg_port_cfg* g_port_cfg;
  pthread_mutex_t cfg_mutex;
  tommy_hashlin g_type_hash;
  tommy_hashlin g_appid_hash;
  tommy_list g_app_lst;
}pg_gbl_cfg;


/* Globals */
extern pg_gbl_cfg* g_pg_gbl_cfg;

/* Internal APIs */

p4_pd_status_t pktgen_app_cfg(int dev_id, uint32_t app_id, p4_pd_pktgen_app_cfg* cfg); 
void pktgen_port_flap(int port, bool status);
void pktgen_init(void);
void pktgen_start(int dev);
void pktgen_stop(int dev);
void pktgen_cleanup(void);
void pktgen_alloc_port_cfg(int dev);

void pktgen_snoop_recirc(int dev, int port, uint8_t* pkt, uint32_t len);
bool pktgen_is_recirc_en(int dev, int port);

uint16_t pktgen_get_pipe(uint32_t port);
uint16_t pktgen_get_port(uint32_t port);


pg_counter_t* pktgen_get_stats(int dev_id, uint32_t app_id);
p4_pd_status_t pktgen_app_arm(int dev_id, uint32_t app_id, bool en); 

#endif
