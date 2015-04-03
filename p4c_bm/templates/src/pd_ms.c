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

#include <p4_sim/pd.h>
#include "pd_ms.h"

#include <Judy.h>
#include <string.h>
#include <assert.h>

#define NUM_DEVICES 256

/* typedef Pvoid_t mbr_to_act_map_t; */
/* typedef Pvoid_t mbr_to_grp_list_map_t; */

struct p4_pd_ms_table_state_s {
  Pvoid_t devices_mbr_to_grp_list[NUM_DEVICES];
  Pvoid_t devices_mbr_to_act[NUM_DEVICES];
  Pvoid_t devices_grp_to_act[NUM_DEVICES];
};

//:: _action_profiles = set()
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   if act_prof in _action_profiles: continue
//::   _action_profiles.add(act_prof)
p4_pd_ms_table_state_t ms_${act_prof}_state;
//:: #endfor

void p4_pd_ms_init(void) {
//:: _action_profiles = set()
//:: for table, t_info in table_info.items():
//::   act_prof = t_info["action_profile"]
//::   if act_prof is None: continue
//::   if act_prof in _action_profiles: continue
//::   _action_profiles.add(act_prof)
  memset(ms_${act_prof}_state.devices_mbr_to_grp_list, 0, NUM_DEVICES * sizeof(Pvoid_t));
  memset(ms_${act_prof}_state.devices_mbr_to_act, 0, NUM_DEVICES * sizeof(Pvoid_t));
  memset(ms_${act_prof}_state.devices_grp_to_act, 0, NUM_DEVICES * sizeof(Pvoid_t));
//:: #endfor
}

int p4_pd_ms_add_mbr_to_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			    p4_pd_mbr_hdl_t mbr_hdl, p4_pd_grp_hdl_t grp_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  int Rc_int;
  Pvoid_t *jarray_grps = (Pvoid_t *) PValue;
  J1S(Rc_int, *jarray_grps, grp_hdl);
  return Rc_int;
}

int p4_pd_ms_del_mbr_from_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			      p4_pd_mbr_hdl_t mbr_hdl, p4_pd_grp_hdl_t grp_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  int Rc_int;
  Pvoid_t *jarray_grps = (Pvoid_t *) PValue;
  J1U(Rc_int, *jarray_grps, grp_hdl);
  return Rc_int;
}
			  
void p4_pd_ms_mbr_apply_to_grps(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				p4_pd_mbr_hdl_t mbr_hdl,
				PDMSGrpFn grp_fn, void *aux) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  Word_t Index = 0;
  int Rc_int;
  Pvoid_t *jarray_grps = (Pvoid_t *) PValue;
  J1F(Rc_int, *jarray_grps, Index);
  while(Rc_int) {
    grp_fn(dev_id, mbr_hdl, Index, aux);
    J1N(Rc_int, *jarray_grps, Index);
  }
}

void p4_pd_ms_new_mbr(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_mbr_hdl_t mbr_hdl) {
  PWord_t PValue;
  JLI(PValue, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  assert(!(*PValue));
}

void p4_pd_ms_del_mbr(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_mbr_hdl_t mbr_hdl) {
  PWord_t PValue;
  JLG(PValue, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  Word_t num_freed;
  Pvoid_t *jarray_grps = (Pvoid_t *) PValue;
  J1FA(num_freed, *jarray_grps);
  (void) num_freed;
  int Rc_int;
  JLD(Rc_int, state->devices_mbr_to_grp_list[dev_id], mbr_hdl);
  assert(Rc_int);
}

p4_pd_act_hdl_t p4_pd_ms_get_mbr_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				     p4_pd_mbr_hdl_t mbr_hdl) {
  p4_pd_act_hdl_t *PValue;
  JLG(PValue, state->devices_mbr_to_act[dev_id], mbr_hdl);
  return *PValue;
}

void p4_pd_ms_set_mbr_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			  p4_pd_mbr_hdl_t mbr_hdl, p4_pd_act_hdl_t act_hdl) {
  p4_pd_act_hdl_t *PValue;
  JLI(PValue, state->devices_mbr_to_act[dev_id], mbr_hdl);
  *PValue = act_hdl;
}

p4_pd_act_hdl_t p4_pd_ms_get_grp_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
				     p4_pd_grp_hdl_t grp_hdl) {
  p4_pd_act_hdl_t *PValue;
  JLG(PValue, state->devices_grp_to_act[dev_id], grp_hdl);
  return *PValue;
}

void p4_pd_ms_set_grp_act(p4_pd_ms_table_state_t *state, uint8_t dev_id,
			  p4_pd_grp_hdl_t grp_hdl, p4_pd_act_hdl_t act_hdl) {
  p4_pd_act_hdl_t *PValue;
  JLI(PValue, state->devices_grp_to_act[dev_id], grp_hdl);
  *PValue = act_hdl;
}

/* not very efficient, I have to loop over all members */
void p4_pd_ms_del_grp(p4_pd_ms_table_state_t *state, uint8_t dev_id,
		      p4_pd_grp_hdl_t grp_hdl) {
  Word_t Index = 0;
  PWord_t PValue;
  int Rc_int;
  Pvoid_t *jarray_grps;
  JLF(PValue, state->devices_mbr_to_grp_list[dev_id], Index);
  while(PValue != NULL) {
    jarray_grps = (Pvoid_t *) PValue;
    J1U(Rc_int, *jarray_grps, grp_hdl);
    JLN(PValue, state->devices_mbr_to_grp_list[dev_id], Index);
  }
}
