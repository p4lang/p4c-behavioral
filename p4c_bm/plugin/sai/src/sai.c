/*
* sai.c
*   base implementation
*
*/

#include <stdio.h>
#include <string.h>
#include <memory.h>

#include <p4_sim/sai.h>
#include <p4_sim/sai_templ.h>
#include <p4_sim/pd.h>

extern p4_pd_sess_hdl_t sess_hdl ;
extern p4_pd_dev_target_t dev_tgt;
extern uint8_t dev_id;

extern sai_switch_api_t switch_api;
extern sai_port_api_t port_api;
extern sai_fdb_api_t fdb_api;
extern sai_router_interface_api_t *router_interface_api;
extern sai_virtual_router_api_t *virtual_router_api;
extern sai_route_api_t *route_api;
extern sai_next_hop_api_t *next_hop_api;
extern sai_neighbor_api_t *neighbor_api;


/*
const char *sai_p4_profile_get_value(_In_ sai_switch_profile_id_t profile_id,
                                     _In_ const char* variable)
{
    return variable;
}


int sai_p4_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id,
                                  _Out_ const char** variable,
                                  _Out_ const char** value)
{
    return 0;
}


static service_method_table_t sai_p4_service_table = {
    .profile_get_value = sa_p4_profile_get_value,
    .profile_get_next_value = sai_p4_profile_get_next_value
};
*/

p4_pd_status_t
p4_mac_learn_notify_cb(p4_pd_sess_hdl_t sess_hdl,
                       p4_pd_sai_p4_mac_learn_digest_digest_msg_t *msg,
                       void *client_data)
{
    unsigned int i=0;
    p4_pd_entry_hdl_t entry_hdl;
    p4_pd_sai_p4_learn_notify_match_spec_t match_spec;
    p4_pd_sai_p4_mac_learn_digest_digest_entry_t *entry = msg->entries;
    p4_pd_sai_p4_fdb_match_spec_t match_spec1;
    p4_pd_sai_p4_fdb_set_action_spec_t action_spec;

    printf("Learn callback\n");
    memset(&match_spec, 0, sizeof(match_spec));
    memset(&match_spec1, 0, sizeof(match_spec1));
    memset(&action_spec, 0, sizeof(action_spec));
    for(i=0;i<msg->num_entries;i++, entry++) {
        match_spec.intrinsic_metadata_ingress_port = entry->intrinsic_metadata_ingress_port;
        match_spec.ingress_metadata_vlan_id = entry->ingress_metadata_vlan_id;
        memcpy(match_spec.eth_srcAddr, entry->eth_srcAddr, 6);
        p4_pd_sai_p4_learn_notify_table_add_with_nop ( sess_hdl, dev_tgt, &match_spec, &entry_hdl);
        // setup fdb
        if(entry->ingress_metadata_learning > 1) {
            match_spec1.ingress_metadata_vlan_id = entry->ingress_metadata_vlan_id;
            memcpy(match_spec1.eth_dstAddr, entry->eth_srcAddr, 6);
            action_spec.action_port_id = entry->intrinsic_metadata_ingress_port;
            p4_pd_sai_p4_fdb_table_add_with_fdb_set (sess_hdl, dev_tgt, &match_spec1, &action_spec, &entry_hdl);
        }
    }
    return 0;
}

sai_status_t sai_api_initialize(
    _In_ uint64_t flags,
    _In_ const service_method_table_t* services
    )
{
    p4_pd_entry_hdl_t entry_hdl;
    p4_pd_sai_p4_router_interface_set_default_action_router_interface_miss(sess_hdl, dev_tgt, &entry_hdl);
    p4_pd_sai_p4_learn_notify_set_default_action_generate_learn_notify(sess_hdl, dev_tgt, &entry_hdl);

    // setup a learn callback
    p4_pd_sai_p4_mac_learn_digest_register(sess_hdl, (uint8_t)dev_id, p4_mac_learn_notify_cb, NULL);
    return 0;
}

sai_status_t sai_api_query(
    _In_ sai_api_t sai_api_id,
    _Out_ void** api_method_table
    )
{
    switch(sai_api_id) {
        case SAI_API_SWITCH:
            *api_method_table = &switch_api;
            break;
        case SAI_API_PORT:
            *api_method_table = &port_api;
            break;
        case SAI_API_FDB:
            *api_method_table = &fdb_api;
            break;
        case SAI_API_VIRTUAL_ROUTER:
            *api_method_table = &virtual_router_api;
            break;
        case SAI_API_ROUTE:
            *api_method_table = &route_api;
            break;
        break;
        case SAI_API_NEXT_HOP:
            *api_method_table = &next_hop_api;
            break;
        break;
        case SAI_API_ROUTER_INTERFACE:
            {
                *api_method_table = &router_interface_api;
            }
            break;
        case SAI_API_NEIGHBOR:
                *api_method_table = &neighbor_api;
            break;
        case SAI_API_VLAN:
        break;
        case SAI_API_NEXT_HOP_GROUP:
        break;
        case SAI_API_QOS:
        break;
        case SAI_API_ACL:
        break;
        case SAI_API_HOST_INTERFACE:
        break;
        default:
            return -1;
    }
    return 0;
}

sai_status_t sai_api_uninitialize( void)
{
    return 0;
}


sai_status_t sai_log_set(
    _In_ sai_api_t sai_api_id,
    _In_ sai_log_level_t log_level
    )
{
    return 0;
}





