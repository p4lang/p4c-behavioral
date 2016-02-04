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

#ifndef _DEVPORT_MGR_H
#define _DEVPORT_MGR_H

#include "pd_static.h"

p4_pd_status_t
p4_pd_devport_mgr_add_port(uint32_t dev_id, uint32_t port_id,
    uint32_t  port_width, bool autoneg_enable,
    uint32_t port_speeds, uint32_t port_fec_types,
    uint32_t tx_pause, uint32_t rx_pause);

p4_pd_status_t
p4_pd_devport_mgr_remove_port(uint32_t dev_id, uint32_t port_id);

#endif
