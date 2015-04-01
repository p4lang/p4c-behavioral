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

#ifndef _RMT_ACTION_PROFILES_H
#define _RMT_ACTION_PROFILES_H

#include "phv.h"
#include "enums.h"

#include <pthread.h>
#include <Judy.h>

#define ACT_PROF_DEFAULT_INIT_SIZE 16384 // entries

void action_profiles_init(void);

//:: for act_prof, ap_info in action_profiles.items():
int action_profiles_add_entry_${act_prof}(char *action_data);
//:: #endfor

//:: for act_prof, ap_info in action_profiles.items():
int action_profiles_delete_entry_${act_prof}(int entry_index);
//:: #endfor

//:: for act_prof, ap_info in action_profiles.items():
int action_profiles_modify_entry_${act_prof}(int entry_index, char *action_data);
//:: #endfor

//:: for act_prof, ap_info in action_profiles.items():
char *action_profiles_get_data_${act_prof}(phv_data_t *phv, char *id);
//:: #endfor

int action_profiles_create_group(rmt_act_prof_t act_prof_e);
int action_profiles_delete_group(rmt_act_prof_t act_prof_e, int index);
int action_profiles_add_member_to_group(rmt_act_prof_t act_prof_e,
					int grp_index, int mbr_index);
int action_profiles_delete_member_from_group(rmt_act_prof_t act_prof_e,
					     int grp_index, int mbr_index);

void action_profiles_clean_all(void);

#endif
