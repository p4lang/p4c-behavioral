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

#ifndef _RMT_LF_H
#define _RMT_LF_H

#include <p4_sim/pd.h>
#include "enums.h"
#include "phv.h"

//:: pd_prefix = ["p4_pd", p4_prefix]
//:: lf_prefix = ["lf", p4_prefix]
//:: lf_set_learning_timeout = "_".join(lf_prefix + ["set_learning_timeout"])
void
${lf_set_learning_timeout}(const long int usecs);

//:: for lq in learn_quanta:
p4_pd_status_t
${lq["lf_register_fn"]}(p4_pd_sess_hdl_t sess_hdl, ${lq["cb_fn_type"]} cb_fn, void *client_data);

p4_pd_status_t
${lq["lf_deregister_fn"]}(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
${lq["lf_notify_ack_fn"]}(p4_pd_sess_hdl_t sess_hdl, ${lq["msg_type"]} *digest_msg);
//:: #endfor

void lf_init();

int lf_receive(phv_data_t *phv, const rmt_field_list_t field_list);

void lf_clean_all();
#endif /* _RMT_LF_H */
