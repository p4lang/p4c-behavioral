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

#include "action_profiles.h"
#include "enums.h"
#include "phv.h"
#include "calculations.h"

#include <string.h>
#include <assert.h>

typedef struct act_prof_grp_s {
  Pvoid_t members;
  int count;  /* I could use the Judy function as weel */
} act_prof_grp_t;

typedef struct action_profile_s {
  void *entries_array;
  int max_size;
  Pvoid_t jarray_used;
  Pvoid_t jlarray_groups;
  pthread_rwlock_t lock;
} action_profile_t;

action_profile_t action_profiles_array[RMT_ACT_PROF_COUNT + 1];

//:: for act_prof, ap_info in action_profiles.items():
//::   action_data_width = ap_info["action_data_byte_width"]
static void ${act_prof}_init(void){
  action_profile_t *act_prof = &action_profiles_array[RMT_ACT_PROF_${act_prof}];
//::   ap_size = ap_info["size"]
//::   if not ap_size or ap_size <= 1: ap_size = "ACT_PROF_DEFAULT_INIT_SIZE"
  act_prof->entries_array = malloc(${ap_size} * ${action_data_width});

  act_prof->max_size = ${ap_size};
  act_prof->jarray_used = (Pvoid_t) NULL;
  act_prof->jlarray_groups = (Pvoid_t) NULL;

  pthread_rwlock_init(&act_prof->lock, NULL);
}

//:: #endfor

/* TODO: this is shared with tables code, put it in Judy wrapper file ?*/

static inline int get_and_set_index(action_profile_t *act_prof) {
  Word_t index = 0;
  int Rc_int;
  J1FE(Rc_int, act_prof->jarray_used, index); // Judy1FirstEmpty()
  if(index < act_prof->max_size) {
    J1S(Rc_int, act_prof->jarray_used, index); // Judy1Set()
    return index;
  }
  else {
    return -1;
  }
}

static inline int unset_index(action_profile_t *act_prof, Word_t index) {
  int Rc_int;
  J1U(Rc_int, act_prof->jarray_used, index); // Judy1Unset()
  return Rc_int;
}

static inline int test_index(const Pvoid_t *jarray, Word_t index) {
  int Rc_int;
  J1T(Rc_int, *jarray, index);
  return Rc_int;
}

/* We don't really need auto-generated code here */

//:: for act_prof, ap_info in action_profiles.items():
//::   action_data_width = ap_info["action_data_byte_width"]
int action_profiles_add_entry_${act_prof}(char *action_data){
  action_profile_t *act_prof = &action_profiles_array[RMT_ACT_PROF_${act_prof}];
  pthread_rwlock_wrlock(&act_prof->lock);
  int empty_index = get_and_set_index(act_prof);
  RMT_LOG(P4_LOG_LEVEL_TRACE, "${act_prof}: adding member at index %d\n", empty_index);
  assert(empty_index >= 0);
  char *dst = (char *) act_prof->entries_array + empty_index * ${action_data_width};
  memcpy(dst, action_data, ${action_data_width});
  pthread_rwlock_unlock(&act_prof->lock);
  return empty_index;
}

//:: #endfor

//:: for act_prof, ap_info in action_profiles.items():
//::   action_data_width = ap_info["action_data_byte_width"]
int action_profiles_delete_entry_${act_prof}(int entry_index){
  action_profile_t *act_prof = &action_profiles_array[RMT_ACT_PROF_${act_prof}];
  pthread_rwlock_wrlock(&act_prof->lock);
  int result = !unset_index(act_prof, entry_index);
  pthread_rwlock_unlock(&act_prof->lock);
  return result;
}

//:: #endfor

//:: for act_prof, ap_info in action_profiles.items():
//::   action_data_width = ap_info["action_data_byte_width"]
int action_profiles_modify_entry_${act_prof}(int entry_index, char *action_data){
  action_profile_t *act_prof = &action_profiles_array[RMT_ACT_PROF_${act_prof}];
  pthread_rwlock_wrlock(&act_prof->lock);
  if(!test_index(&act_prof->jarray_used, entry_index)) {
    pthread_rwlock_unlock(&act_prof->lock);
    return -1;
  }
  RMT_LOG(P4_LOG_LEVEL_TRACE, "${act_prof}: modifying member at index %d\n", entry_index);
  char *dst = (char *) act_prof->entries_array + entry_index * ${action_data_width};
  memcpy(dst, action_data, ${action_data_width});  
  pthread_rwlock_unlock(&act_prof->lock);
  return 0;
}

//:: #endfor

static inline act_prof_grp_t *get_grp(action_profile_t *act_prof, int index) {
  PWord_t PValue;
  JLG(PValue, act_prof->jlarray_groups, index);
  assert(PValue);
  return (act_prof_grp_t *) *PValue;
}

static inline int get_member_by_count(act_prof_grp_t *grp, int count) {
  int Rc_int;
  Word_t index;
  J1BC(Rc_int, grp->members, count, index);
  assert(Rc_int);
  return index;
}

/* I'm trying something here: not using generated code for those functions : */

int action_profiles_create_group(rmt_act_prof_t act_prof_e){
  action_profile_t *act_prof = &action_profiles_array[act_prof_e];
  pthread_rwlock_wrlock(&act_prof->lock);
  Word_t index = 0;
  int Rc_int;
  act_prof_grp_t *grp = malloc(sizeof(act_prof_grp_t));
  memset(grp, 0, sizeof(act_prof_grp_t));
  JLFE(Rc_int, act_prof->jlarray_groups, index);
  PWord_t PValue;
  JLI(PValue, act_prof->jlarray_groups, index);
  *PValue = (Word_t) grp;
  pthread_rwlock_unlock(&act_prof->lock);
  return index;
}

int action_profiles_delete_group(rmt_act_prof_t act_prof_e, int index){
  action_profile_t *act_prof = &action_profiles_array[act_prof_e];
  pthread_rwlock_wrlock(&act_prof->lock);
  int Rc_int;
  JLD(Rc_int, act_prof->jlarray_groups, index);
  pthread_rwlock_unlock(&act_prof->lock);
  return (Rc_int != 1);
}

int action_profiles_add_member_to_group(rmt_act_prof_t act_prof_e,
					int grp_index, int mbr_index){
  action_profile_t *act_prof = &action_profiles_array[act_prof_e];
  pthread_rwlock_wrlock(&act_prof->lock);
  PWord_t PValue;
  JLG(PValue, act_prof->jlarray_groups, grp_index);
  assert(PValue);
  act_prof_grp_t *grp = (act_prof_grp_t *) *PValue;
  int Rc_int;
  J1S(Rc_int, grp->members, mbr_index);
  if(Rc_int) {
    /* index was previously unset */
    grp->count++;
  }
  pthread_rwlock_unlock(&act_prof->lock);
  return (Rc_int != 1);
}

int action_profiles_delete_member_from_group(rmt_act_prof_t act_prof_e,
					     int grp_index, int mbr_index){
  action_profile_t *act_prof = &action_profiles_array[act_prof_e];
  pthread_rwlock_wrlock(&act_prof->lock);
  PWord_t PValue;
  JLG(PValue, act_prof->jlarray_groups, grp_index);
  assert(PValue);
  act_prof_grp_t *grp = (act_prof_grp_t *) *PValue;
  int Rc_int;
  J1U(Rc_int, grp->members, mbr_index);
  if(Rc_int) {
    /* index was previously set */
    grp->count--;
  }
  pthread_rwlock_unlock(&act_prof->lock);
  return (Rc_int != 1);
}


//:: for act_prof, ap_info in action_profiles.items():
//::   action_data_width = ap_info["action_data_byte_width"]
char *action_profiles_get_data_${act_prof}(phv_data_t *phv, char *id) {
  action_profile_t *act_prof = &action_profiles_array[RMT_ACT_PROF_${act_prof}];
  int index = *(int *) id;
  index &= 0x00FFFFFF;
  pthread_rwlock_rdlock(&act_prof->lock);
//::   selection_key = ap_info["selection_key"]
//::   if selection_key is not None:
//::     c_info = field_list_calculations[selection_key]
//::     bytes_output = c_info["output_phv_bytes"]
  char meta = id[3];
  int grp = meta & 0x80;
  if(grp) {
    act_prof_grp_t *grp = get_grp(act_prof, index);
    uint8_t computed[${bytes_output}];
    calculations_${selection_key}(phv, computed);
    /* computed is in network byte order !!!! */
    /* we are only interested in the last 4 bytes */
    assert(grp->count);
    int member_count = ((computed[0] << 24) + (computed[1] << 16) +
			(computed[2] << 8) + (computed[3])) % grp->count;
    index = get_member_by_count(grp, member_count + 1);
  }
//::   #endif
  char *dst = (char *) act_prof->entries_array + index * ${action_data_width};  
  pthread_rwlock_unlock(&act_prof->lock);
  return dst;
}

//:: #endfor

void action_profiles_clean_all(void) {
  rmt_act_prof_t act_prof_e;
  action_profile_t *act_prof;
  Word_t bytes_freed;
  Word_t grp_index;
  PWord_t grp_ptr;
  act_prof_grp_t *grp;
  RMT_ACT_PROF_FOREACH(act_prof_e) {
    act_prof = &action_profiles_array[act_prof_e];
    pthread_rwlock_wrlock(&act_prof->lock);

    J1FA(bytes_freed, act_prof->jarray_used);

    grp_index = 0;;
    JLF(grp_ptr, act_prof->jlarray_groups, grp_index);
    while(grp_ptr != NULL) {
      grp = (act_prof_grp_t *) *grp_ptr;
      J1FA(bytes_freed, grp->members);
      grp->count = 0;
      JLN(grp_ptr, act_prof->jlarray_groups, grp_index);
    }

    JLFA(bytes_freed, act_prof->jlarray_groups);

    pthread_rwlock_unlock(&act_prof->lock);
  }
}

void action_profiles_init(void) {
//:: for act_prof, ap_info in action_profiles.items():
  ${act_prof}_init();
//:: #endfor  
}
