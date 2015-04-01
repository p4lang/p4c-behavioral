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

#include "rmt_internal.h"

#include <assert.h>
#include <pthread.h>
#include <Judy.h>

static Pvoid_t used_session_handles;
static pthread_mutex_t lock;

p4_pd_status_t
p4_pd_init(void) {
  pthread_mutex_init(&lock, NULL);
  used_session_handles = (Pvoid_t) NULL;
  return 0;
}

void
p4_pd_cleanup(void) {
  Word_t Rc_word;
  J1FA(Rc_word, used_session_handles);
}

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size) {
  pthread_mutex_lock(&lock);

  int Rc_int;
  Word_t index = 0;
  J1FE(Rc_int, used_session_handles, index);
  *sess_hdl = (p4_pd_sess_hdl_t)index;

  J1S(Rc_int, used_session_handles, index);
  assert(1 == Rc_int);

  pthread_mutex_unlock(&lock);

  return 0;
}

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl) {
  pthread_mutex_lock(&lock);

  int Rc_int;
  J1U(Rc_int, used_session_handles, (Word_t)sess_hdl);
  if (0 == Rc_int) {
    RMT_LOG(P4_LOG_LEVEL_ERROR, "Cannot find session handle %u\n", (uint32_t)sess_hdl);
  }

  pthread_mutex_unlock(&lock);

  return (p4_pd_status_t)(1 == Rc_int ? 0 : 1);
}

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri) {
  return 0;
}

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl) {
  return 0;
}

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl) {
  return 0;
}

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp) {
  return 0;
}

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl) {
  return 0;
}

uint16_t
p4_pd_dev_port_to_pipe_id(uint16_t dev_port_id)
{
    return ((dev_port_id >> 7) & 0x3);
}
