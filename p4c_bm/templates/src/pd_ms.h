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

#ifndef _RMT_PD_MS_H
#define _RMT_PD_MS_H

#include "actions.h"

void p4_pd_ms_init(void);

typedef ActionFn p4_pd_act_hdl_t;
typedef struct p4_pd_ms_table_state_s p4_pd_ms_table_state_t;

//:: _action_profiles = set()
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   if act_prof in _action_profiles: continue
//::   _action_profiles.add(act_prof)
extern p4_pd_ms_table_state_t ms_${act_prof}_state;
//:: #endfor

void p4_pd_ms_new_mbr(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_mbr_hdl_t mbr_hdl);

void p4_pd_ms_del_mbr(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_mbr_hdl_t mbr_hdl);

int p4_pd_ms_add_mbr_to_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			    p4_pd_mbr_hdl_t mbr_hdl, p4_pd_grp_hdl_t grp_hdl);

int p4_pd_ms_del_mbr_from_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			      p4_pd_mbr_hdl_t mbr_hdl, p4_pd_grp_hdl_t grp_hdl);

typedef void (*PDMSGrpFn)(uint8_t dev_id,
			  p4_pd_mbr_hdl_t mbr_hdl,
			  p4_pd_grp_hdl_t grp_hdl,
			  void *aux);

void p4_pd_ms_mbr_apply_to_grps(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				p4_pd_mbr_hdl_t mbr_hdl,
				PDMSGrpFn grp_fn, void *aux);

p4_pd_act_hdl_t p4_pd_ms_get_mbr_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				     p4_pd_mbr_hdl_t mbr_hdl);

void p4_pd_ms_set_mbr_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			  p4_pd_mbr_hdl_t mbr_hdl, p4_pd_act_hdl_t act_hdl);

p4_pd_act_hdl_t p4_pd_ms_get_grp_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				     p4_pd_grp_hdl_t grp_hdl);

void p4_pd_ms_set_grp_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			  p4_pd_grp_hdl_t grp_hdl, p4_pd_act_hdl_t act_hdl);

void p4_pd_ms_del_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_grp_hdl_t grp_hdl);

#endif
