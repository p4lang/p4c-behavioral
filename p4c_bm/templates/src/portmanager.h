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

#ifndef __PORTMANAGER_H
#define __PORTMANAGER_H

#include <stdbool.h>
#include <p4_sim/pd_static.h>

#define PORT_COUNT_PER_CHIP_MAX 456
#define MAX_CHIPS 8
#define PORT_COUNT_MAX (MAX_CHIPS * PORT_COUNT_PER_CHIP_MAX) + 1


typedef void (*port_fn)(int, bool);
void portmgr_init(void);
void portmgr_cleanup(void);
void portmgr_register_cb(port_fn func);
p4_pd_status_t p4_port_add(uint32_t port);
p4_pd_status_t p4_port_remove(uint32_t port);

#endif
