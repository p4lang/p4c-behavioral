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
/*
* sai_templ.c
*   Implementation files
*   Auto generated files - please do not edit
*/

#include <stdio.h>
#include <string.h>

#include <p4_sim/sai_templ.h>
#include <p4_sim/pd.h>
p4_pd_sess_hdl_t sess_hdl ;
p4_pd_dev_target_t dev_tgt;
uint8_t dev_id;
 
#define SAI_LOG(f_, ...) printf((f_), __VA_ARGS__)

// Need to make sure the space from status is carved for this
// Need to come up with a better scheme!
#define P4_CASE_NOT_SUPPORTED (-1000)

/*
*   utility functions for manipulating PD entry handle return
*/
#define TEST 0

#include <p4utils/xxhash.h>
#include <p4utils/tommyhashdyn.h>
#define hashlittle XXH32

static tommy_hashdyn entry_hash;

// each node stored in hash needs size and pointer
struct entry_hash_node {
    tommy_hashdyn_node node;
    unsigned char key[32];
    unsigned int size;
    unsigned int entry_handle;
};

void _init_entry_index_hashmap()
{
    tommy_hashdyn_init(&entry_hash);
}


static int _add_entry_index(int type, void *entry, unsigned int size, struct entry_hash_node *data)
{
    tommy_hash_t hash=hashlittle(entry, size, type);
    tommy_hashdyn_insert(&entry_hash, &(data->node), data, hash);
    return 0;
}

static int entry_hash_cmp(const void *a1, const void *a2)
{
    return memcmp(a1, ((struct entry_hash_node *)a2)->key, ((struct entry_hash_node *)a2)->size);
}

static int _get_entry_index(int type, void *entry, unsigned int size)
{
    struct entry_hash_node  *h;
    tommy_hash_t hash=hashlittle(entry, size, type);
     h = tommy_hashdyn_search(&entry_hash, entry_hash_cmp, entry, hash);
    if(h)
        return h->entry_handle;
	return -1;
}

int _del_entry_index(int type, void *entry, unsigned int size)
{
    tommy_hash_t hash=hashlittle(entry, size, type);
    void *p = tommy_hashdyn_remove (&entry_hash, entry_hash_cmp, entry, hash);
    if(p) {
        free(p);
        return 0;
    }
    return -1;
}

#if TEST

#include <stdio.h>
#include <string.h>
#include <memory.h>
int _test_hash_main(int argc, char **argv)
{
    unsigned char key[10] = {0x0, 0x3, 0x7, 0x9, 0x2, 0x4, 0x6};
    unsigned char key1[20] = {0x0, 0x3, 0x7, 0x9, 0x2, 0x4, 0x6};
    struct entry_hash_node *data = NULL;
    struct entry_hash_node *data1 = NULL;
    int e = -1;
    _init_entry_index_hashmap();
    data = malloc(sizeof(struct entry_hash_node));
    memcpy(data->key, key, 10);
    data->size = 10;
    data->entry_handle = 1234;
    data1 = malloc(sizeof(struct entry_hash_node));
    memcpy(data1->key, key1, 20);
    data1->size = 20;
    data1->entry_handle = 7890;
    if(data && data1) {
        _add_entry_index(1, key, 10, data);
        _add_entry_index(2, key1, 20, data1);
        key[9] = 0;
        if((e = _get_entry_index(2, key1, 20)) != -1) {
            printf("Found key: index %d\n", e);
        }
        else {
            printf("Entry not found\n");
        }
        _del_entry_index(1, key, 10);
        _del_entry_index(2, key1, 10);
    }
    else 
        printf("Failed alloc\n");
    return 0;
}
#endif

typedef enum {
    ADD,
    MOD,
    DEL
} p4_table_opcode_t;


//:: def get_type(byte_width):
//::   if byte_width == 1:
//::     return "uint8_t"
//::   elif byte_width == 2:
//::     return "uint16_t"
//::   elif byte_width <= 4:
//::     return "uint32_t"
//::   else:
//::     return "uint8_t *"
//::   #endif
//:: #enddef
//::
//:: # match_fields is list of tuples (name, type)
//:: def gen_match_params(match_fields, field_info):
//::   params = []
//::   for field, type in match_fields:
//::     if type == "valid":
//::       params += [(field + "_valid", 1)]
//::       continue
//::     #endif
//::     f_info = field_info[field]
//::     bytes_needed = (f_info["bit_width"] + 7 ) / 8
//::     params += [(field, bytes_needed)]
//::     if type == "lpm": params += [(field + "_prefix_length", 2)]
//::     if type == "ternary": params += [(field + "_mask", bytes_needed)]
//::   #endfor
//::   return params
//:: #enddef
//::
//:: def gen_action_params_name(names, byte_widths, _get_type = get_type):
//::   params = []
//::   for name, width in zip(names, byte_widths):
//::     params += [("action_" + name, width, name)]
//::   #endfor
//::   return params
//:: #enddef
//::
//::

//:: for table, t_info in table_info.items():
/*
*   ${table.upper()} Table API Implementation
*/

//::   act_prof_name = t_info["action_profile"]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   if act_prof_name is not None:
//::     # Get the  actions associated with act_prof_name
//::     parent_table = table
//::     parent_actions = t_info["actions"]
//::     table = act_prof_name
//::     act_prof = action_profiles[act_prof_name]
//::     t_info["actions"] = act_prof["actions"]
//::   #endif
//::   if has_match_spec:

void p4_sai_entry_to_${table}_match_spec(
    const sai_${table}_entry_t* ${table}_entry,
//::       if act_prof_name is not None:
    ${p4_pd_prefix}${parent_table}_match_spec_t* match_spec)
{
//::       else:
    ${p4_pd_prefix}${table}_match_spec_t* match_spec)
{
//::       #endif
//::       m_init += "\n    memset(&match_spec, 0, sizeof(match_spec));"
//::       m_init += "\n    p4_sai_entry_to_" + table + "_match_spec(" + table + "_entry" + ", &match_spec);"
    memcpy(match_spec, ${table}_entry, sizeof(sai_${table}_entry_t));
}

//::   #endif
//::   has_any_action_spec = False
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["sess_hdl",
//::               "dev_tgt"]
//::     if has_match_spec:
//::       params += ["match_spec"]
//::     #endif
//::     if match_type == "ternary":
//::       params += ["int priority"]
//::     #endif
//::     if has_action_spec:
//::       has_any_action_spec = True
//::       params += ["&action_spec"]
//::     #endif
//::     if t_info["support_timeout"]:
//::       params += ["uint32_t ttl"]
//::     #endif
//::     params += ["&entry_hdl"]
//::     param_str = ",\n     ".join(params)
//::     del_name = p4_pd_prefix + table + "_table_delete"
//::     mod_params = ["sess_hdl", "dev_id"]
//::     del_params = ["sess_hdl", 
//::               "dev_id"] 
//::     if match_type == "ternary":
//::       del_params += ["int priority"]
//::     #endif
//::     del_params += ["entry_hdl"]
//::     if act_prof_name is not None:
//::         add_name = p4_pd_prefix + parent_table + "_add_entry"
//::         mod_name = add_name
//::     else:
//::         add_name = p4_pd_prefix + table + "_table_add_with_" + action
//::         mod_name = p4_pd_prefix + table + "_table_modify_with_" + action
//::     #endif
//::     del_param_str = ",\n       ".join(del_params) 
//::     mod_param_str = ",\n       ".join(mod_params) 
//::     local_str1 = ""
//::     local_str2 = ""
//::     m_init = ""
//::     if has_match_spec:
//::       if act_prof_name is not None:
//::         local_str1 += p4_pd_prefix + parent_table + "_match_spec_t match_spec;\n"
//::       else:
//::         local_str1 += p4_pd_prefix + table + "_match_spec_t match_spec;\n"
//::       #endif
//::       m_init += "\n    memset(&match_spec, 0, sizeof(match_spec));"
//::       m_init += "\n    p4_sai_entry_to_" + table + "_match_spec(" + table + "_entry" + ", &match_spec);"
//::     #endif
//::     if has_action_spec:
//::       local_str2 += p4_pd_prefix + action + "_action_spec_t action_spec;\n"
//::       local_str2 += "\n    memset(&action_spec, 0, sizeof(action_spec));"

p4_pd_status_t  p4_sai_attribute_to_${table}_action_spec_${action}(p4_table_opcode_t op_code, void *match_spec, uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list)
{
    p4_pd_status_t status=0; 
    unsigned int i=0;
    p4_pd_entry_hdl_t entry_hdl;
    ${p4_pd_prefix}${action}_action_spec_t action_spec;

    memset(&action_spec, 0, sizeof(action_spec));
    for(i=0;i<attr_count;i++) {
        SAI_LOG(" ATTR Id %d\n",  ((sai_attribute_t *)(attr_list+i))->id);
//::       action_params = gen_action_params_name(a_info["param_names"],
//::                                     a_info["param_byte_widths"])
//::       if has_action_spec:
        switch( ((sai_attribute_t *)((attr_list+i)))->id) {
//::         for name, width, p in action_params:
            case SAI_${table.upper()}_ATTR_${p.upper()}:
//::           if width > 4:
                memcpy(&(action_spec.${name}), &((attr_list+i)->value), ${width});
//::           else:
//::             type_ = get_type(width)
                action_spec.${name} =  *(${type_} *)(&((attr_list+i)->value));
//::           #endif
            break;
//::         #endfor

            default:
            return P4_CASE_NOT_SUPPORTED;
        }
//::       #endif
    }
//::       if act_prof_name is None:
    switch(op_code) {
        case ADD:
            {
                status = ${add_name} (
                        ${param_str} 
                        ); 
//::            if has_match_spec:
                {
                    struct entry_hash_node *data = malloc(sizeof(struct entry_hash_node));
                    if(data) {
                        memcpy(data->key, match_spec, sizeof(sai_${table}_entry_t));
                        data->size = sizeof(sai_${table}_entry_t);
                        data->entry_handle = entry_hdl;
                        _add_entry_index(0, match_spec, sizeof(sai_${table}_entry_t), data);
//                        SAI_LOG("Add hdl 0x%0x\n", data->entry_handle);
                    }
                }
//::            #endif
            }
            break;
        case MOD:
            {
//::            if has_match_spec:
                entry_hdl = _get_entry_index(0, match_spec, sizeof(sai_${table}_entry_t));
//                SAI_LOG(" mod hdl 0x%0x\n", entry_hdl);
//::            else:
                entry_hdl = 0; // always 0 as there is only one record in table
//::            #endif
                /*
                status = ${mod_name} (
                         ${mod_param_str}, entry_hdl
//::     if has_action_spec:
                         , &action_spec
//::     #endif
                         );
                */
            }
            break;
            case DEL:
            {
//::            if has_match_spec:
                entry_hdl = _get_entry_index(0, match_spec, sizeof(sai_${table}_entry_t));
//                SAI_LOG(" del hdl 0x%0x\n", entry_hdl);
//::            else:
                entry_hdl = 0; // always 0 as there is only one record in table
//::            #endif
                status = ${del_name} (
                             ${del_param_str} 
                            );
            }
            break;
    }
//::       #endif
    return status;
}

//::     else:
/*
TODO Handle the action_profile case here
*/
//::     #endif
//::     
//::   #endfor
//::   if len(t_info["actions"]) > 0 and has_any_action_spec:

p4_pd_status_t  p4_sai_attribute_to_${table}_action_spec(p4_table_opcode_t op_code, 
                    void *match_spec,
                    uint32_t attr_count,
                    _In_ const sai_attribute_t *attr_list)
{
    sai_status_t  status=0; 
//    SAI_LOG("%s: %d\n", __FILE__, __LINE__);
    {
        /*
        *   Call appropriate action functions to set the entry
        */
//::        for action in t_info["actions"]:
//::          a_info = action_info[action]
//::          if len(a_info["param_names"]) > 0:
        if((status = p4_sai_attribute_to_${table}_action_spec_${action}(op_code, match_spec, attr_count, (attr_list))) != P4_CASE_NOT_SUPPORTED) {
            return status;
        }
//::          #endif
//::        #endfor
        if(status == P4_CASE_NOT_SUPPORTED) {
            return -1; // NOT implemented!!
        }
    }
    return status;
}

//::   #endif

sai_status_t sai_create_${table}(
//::     if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::     #endif
    _In_ uint32_t attr_count,
    _In_ const sai_attribute_t *attr_list
    )
{
    sai_status_t  status = 0;
    ${local_str1}
    ${m_init}
//::     if has_any_action_spec:
    status = p4_sai_attribute_to_${table}_action_spec(ADD, 
//::        if has_match_spec:
                    &match_spec,
//::        else:
                    NULL,
//::        #endif
                    attr_count, attr_list);
//::     #endif
    return status;
}


sai_status_t sai_remove_${table}(
//::     if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry
//::     #endif
    )
{
    p4_pd_status_t  status = 0;
    ${local_str1}
    ${m_init}
    {
        p4_pd_entry_hdl_t entry_hdl;
//::            if has_match_spec:
                entry_hdl = _get_entry_index(0, &match_spec, sizeof(sai_${table}_entry_t));
//                SAI_LOG(" del hdl 0x%0x\n", entry_hdl);
//::            else:
                entry_hdl = 0; // always 0 as there is only one record in table
//::            #endif
                status = ${del_name} (
                             ${del_param_str} 
                            );
    }
    return status;
}


sai_status_t sai_set_${table}_attribute(
//::     if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::     #endif
    _In_ const sai_attribute_t *attr
    )
{
    p4_pd_status_t  status = 0;
    ${local_str1}
    ${m_init}
//::     if has_any_action_spec:
    p4_sai_attribute_to_${table}_action_spec(MOD, 
//::        if has_match_spec:
                            &match_spec,
//::        else:
                            NULL,
//::        #endif
                            1, attr);
//::     #endif
    // TODO
    /*
    {
    p4_pd_entry_hdl_t entry_hdl;
    status = ${mod_name} (
     ${mod_param_str}, entry_hdl
//::     if has_any_action_spec:
     , &action_spec
//::     #endif
     );
    }
    */

    return status;
}


sai_status_t sai_get_${table}_attribute(
//::     if has_match_spec:
    _In_ const sai_${table}_entry_t* ${table}_entry,
//::     #endif
    _In_ uint32_t attr_count,
    _Inout_ const sai_attribute_t *attr_list
    )
{
    // TODO
    return 0;
}


sai_status_t sai_remove_all_${table}s(void)
{
    // TODO
    return 0;
}

//::   if act_prof_name is not None:
/*
    Member add/delete functions for ${table}
*/
sai_status_t sai_add_${parent_table}_to_${table}(
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ uint32_t ${parent_table}_count,
    _In_ const sai_${table}_${parent_table}_t* ${parent_table}_list
    )
{
    // group table add PD API
    // TODO
    return 0;
}


sai_status_t sai_remove_${parent_table}_from_${table}(
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ uint32_t ${parent_table}_count,
    _In_ const sai_${table}_${parent_table}_t* ${parent_table}_list
    )
{
    // TODO
    return 0;
}

//::   #endif

//::   if t_info["bytes_counter"] is not None or t_info["packets_counter"] is not None:
sai_status_t sai_get_${table}_stats (
    _In_ sai_${table}_entry_t* ${table}_entry,
    _In_ const sai_${table}_stat_counter_t *counter_ids,
    _In_ uint32_t number_of_counters,
    _Out_ uint64_t* counters
    )
{
    // TODO
    return 0;
}


//::   #endif

sai_${table}_api_t ${table}_api = {
    .create_${table} = sai_create_${table},
    .remove_${table} = sai_remove_${table},
    .set_${table}_attribute = sai_set_${table}_attribute,
    .get_${table}_attribute = sai_get_${table}_attribute,
//::   if act_prof_name is not None:
    .add_${parent_table}_to_${table} = sai_add_${parent_table}_to_${table},
    .remove_${parent_table}_from_${table} = sai_remove_${parent_table}_from_${table},
//::   #endif
    .remove_all_${table}s = sai_remove_all_${table}s,
//::   if t_info["bytes_counter"] is not None or t_info["packets_counter"] is not None:
    .get_${table}_stats = sai_get_${table}_stats
//::   #endif
};


//:: #endfor

