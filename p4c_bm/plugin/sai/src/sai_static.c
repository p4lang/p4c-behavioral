#include <p4_sim/sai.h>
#include <p4_sim/sai_templ.h>

static const char *sai_p4_profile_get_value(_In_ sai_switch_profile_id_t profile_id,
                        _In_ const char* variable)
{
        return variable;
}


static int sai_p4_profile_get_next_value(_In_ sai_switch_profile_id_t profile_id,
                                  _Out_ const char** variable,
                                  _Out_ const char** value)
{
        return 0;
}


static service_method_table_t sai_p4_service_table = {
        .profile_get_value = sai_p4_profile_get_value,
        .profile_get_next_value = sai_p4_profile_get_next_value
};

static sai_switch_api_t *switch_api;
static sai_port_api_t *port_api;
static sai_fdb_api_t *fdb_api;
static sai_router_interface_api_t *router_interface_api;
static sai_virtual_router_api_t *virtual_router_api;
static sai_route_api_t *route_api;
static sai_next_hop_api_t *next_hop_api;
static sai_neighbor_api_t *neighbor_api;



extern void _init_entry_index_hashmap(void);

void sai_init( void )
{
    _init_entry_index_hashmap();
    sai_api_initialize(0, &sai_p4_service_table);
}

void sai_simple_setup( void )
{
//    printf("Simple SAI setup\n");
    sai_api_query(SAI_API_SWITCH, (void **)&switch_api);
    if(switch_api) {
        sai_attribute_t attr1[2];
        attr1[0].id = SAI_SWITCH_ATTR_OPER_STATUS;
        attr1[0].value.u32 = 1;
        attr1[1].id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
        attr1[1].value.u32 = 0x22222200;
        switch_api->create_switch(2, attr1);
    }

    sai_api_query(SAI_API_PORT, (void **)&port_api);
    if(port_api) {
        sai_port_entry_t port1 = {1};
        sai_attribute_t attr1[2];
        attr1[0].id = SAI_PORT_ATTR_OPER_STATUS;
        attr1[0].value.u32 = 1;
        attr1[1].id = SAI_PORT_ATTR_FDB_LEARNING;
        attr1[1].value.u32 = 2;
        port_api->create_port(&port1, 2, attr1);
    }

    sai_api_query(SAI_API_ROUTER_INTERFACE, (void **)&router_interface_api);
    if(router_interface_api) {
        sai_router_interface_entry_t router1 = {{0x00, 0x12, 0x34, 0x56, 0x78, 0x90}};
        sai_attribute_t attr1[2];
        attr1[0].id = SAI_ROUTER_INTERFACE_ATTR_PORT_ID;
        attr1[0].value.u32 = 2;
        attr1[1].id = SAI_ROUTER_INTERFACE_ATTR_VIRTUAL_ROUTER_ID;
        attr1[1].value.u32 = 10;
        router_interface_api->create_router_interface(&router1, 2, attr1);
    }

    sai_api_query(SAI_API_FDB, (void **)&fdb_api);
    if(fdb_api) {
        sai_fdb_entry_t fdb1 = {0, {0x00, 0x11, 0x11, 0x11, 0x11, 0x11}};
        sai_attribute_t attr1;
        attr1.id = SAI_FDB_ATTR_PORT_ID;
        attr1.value.u32 = 2;
        fdb_api->create_fdb(&fdb1, 1, &attr1);
    }

    sai_api_query(SAI_API_VIRTUAL_ROUTER, (void **)&virtual_router_api);
    if(virtual_router_api) {
        sai_virtual_router_entry_t virtual_router1 = {10};
        sai_attribute_t attr1[2];
        attr1[0].id = SAI_VIRTUAL_ROUTER_ATTR_ADMIN_V4_STATE;
        attr1[0].value.u32 = 1;
        attr1[1].id = SAI_VIRTUAL_ROUTER_ATTR_SRC_MAC_ADDRESS;
        attr1[1].value.u32 = 0x22222200;
        virtual_router_api->create_virtual_router(&virtual_router1, 2, attr1);
    }

    sai_api_query(SAI_API_ROUTE, (void **)&route_api);
    if(route_api) {
        sai_route_entry_t route1 = {10, 0x0a000001};
        sai_attribute_t attr1;
        attr1.id = SAI_ROUTE_ATTR_NEXT_HOP_ID;
        attr1.value.u32 = 1;
        route_api->create_route(&route1, 1, &attr1);
    }

    sai_api_query(SAI_API_NEXT_HOP, (void **)&next_hop_api);
    if(next_hop_api) {
        sai_next_hop_entry_t nexthop1 = {1};
        sai_attribute_t attr1;
        attr1.id = SAI_NEXT_HOP_ATTR_ROUTER_INTERFACE_ID;
        attr1.value.u32 = 2;
        next_hop_api->create_next_hop(&nexthop1, 1, &attr1);
    }

    sai_api_query(SAI_API_NEIGHBOR, (void **)&neighbor_api);
    if(neighbor_api) {
        sai_neighbor_entry_t neighbor1 = {10, 0x0a000001, 2};
        sai_attribute_t attr1[2];
        attr1[0].id = SAI_NEIGHBOR_ATTR_PORT_ID;
        attr1[0].value.u32 = 3;
        attr1[1].id = SAI_NEIGHBOR_ATTR_DST_MAC_ADDRESS;
        attr1[1].value.u32 = 2;
        neighbor_api->create_neighbor(&neighbor1, 2, attr1);
    }
}


