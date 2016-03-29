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

//:: pd_prefix = "p4_pd_" + p4_prefix + "_"
//:: pd_static_prefix = "p4_pd_"
//:: api_prefix = p4_prefix + "_"

#include "devport_mgr.h"

extern "C" {
#include <p4_sim/pd_static.h>
#include <p4_sim/pd_devport_mgr.h>
#include <p4_sim/rmt.h>
}

using boost::shared_ptr;

using namespace  ::devport_mgr_pd_rpc;

class devport_mgrHandler : virtual public devport_mgrIf {
 public:
  devport_mgrHandler() {
  }
  //:: name = "devport_mgr_add_port"
  //:: pd_name = pd_static_prefix + name

  int32_t ${name}(const int32_t dev_id,
      const int32_t port_id,
      const int32_t port_width,
      const bool autoneg_enable,
      const int32_t port_speeds,
      const int32_t port_fec_types,
      const int32_t tx_pause,
      const int32_t rx_pause) {

    return ${pd_name}(dev_id,
        port_id,
        port_width,
        autoneg_enable,
        port_speeds,
        port_fec_types,
        tx_pause,
        rx_pause);
  }

  //:: name = "devport_mgr_remove_port"
  //:: pd_name = pd_static_prefix + name
  int32_t ${name}(const int32_t dev_id, const int32_t port_id) {
    return ${pd_name}(dev_id, port_id);
  }

};

