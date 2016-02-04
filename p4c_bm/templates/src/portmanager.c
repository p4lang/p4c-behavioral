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

#include "portmanager.h"
#include <stdio.h>
#include <pthread.h>
#include <BMI/bmi_port.h>
#include <unistd.h>
#include <p4utils/tommylist.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "rmt_internal.h"

pthread_t port_monitor_th;
extern bmi_port_mgr_t* g_port_mgr;
tommy_list port_list;


struct functor_node {
  tommy_node node;
  port_fn func;
};

typedef struct port_status {
  bool status;
  bool added;
}port_status;

port_status* status_w = NULL;

p4_pd_status_t p4_port_add(uint32_t port_id) {
  if (!status_w || status_w[port_id].added) return P4_PD_FAILURE;
  RMT_LOG(P4_LOG_LEVEL_TRACE, "Adding port: %u\n", port_id);
  status_w[port_id].added = true;
  return P4_PD_SUCCESS;

}
p4_pd_status_t p4_port_remove(uint32_t port_id) {
  if (!status_w || !status_w[port_id].added) return P4_PD_FAILURE;
  RMT_LOG(P4_LOG_LEVEL_TRACE, "Removing port: %u\n", port_id);
  status_w[port_id].added = false;
  return P4_PD_SUCCESS;

}


bool port_is_up(int port) {
  bool is_up = false;
  if (0 >= port || PORT_COUNT_MAX < port) return false;
  if (g_port_mgr == NULL) return false;
  if (bmi_port_interface_is_up(g_port_mgr, port, &is_up)) return false;
  return is_up;

}

void port_handler(int port, bool status) {
  tommy_node* i = tommy_list_head(&port_list);
  while (i) {
    struct functor_node* f_node = i->data;
    f_node->func(port, status);
    i = i->next;
  }
}

void* port_monitor(void* arg) {
  int i = 0;
  status_w = (port_status *)calloc(PORT_COUNT_MAX, sizeof(port_status));
  assert(status_w);
  while (1) {
    usleep(100);
    for (i = 0; i < PORT_COUNT_MAX; i ++) {
      bool x = port_is_up(i);
      if (!x && status_w[i].status) {
        port_handler(i, false);
      }
      if (x && !status_w[i].status) {
        port_handler(i, true);
      }
      status_w[i].status = x;
    }
  }
  free(status_w);
  status_w = NULL;


}


/*
 * Limited to detecting whenever port up/down events happen
 */

void portmgr_init(void) {
  if (g_port_mgr == NULL) return;
  tommy_list_init(&port_list);
  pthread_create(&port_monitor_th, NULL, port_monitor, NULL);
}

void portmgr_register_cb(port_fn func) {
  struct functor_node* f_node = (struct functor_node *)malloc(sizeof(struct functor_node));
  f_node->func = func;
  tommy_list_insert_tail(&port_list, &f_node->node, f_node);
}


void portmgr_cleanup(void) {
  pthread_cancel(port_monitor_th);
  pthread_join(port_monitor_th, NULL);
  tommy_list_foreach(&port_list, free);
}
