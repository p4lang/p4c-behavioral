//:: from common import *
//:: import sys
//:: import os
//:: from of import *
//::
//:: map_mod = __import__(openflow_mapping_mod)
//::
//:: p4_pd_prefix = "p4_pd_" + p4_prefix + "_"
#include "p4_sim/pd_wrappers.h"
#include "p4_sim/pd.h"
#include "p4_sim/pre.h"
#include "p4_sim/ofpat_pipeline.h"
#include "p4ofagent/p4ofagent.h"
#include "p4ofagent/callbacks.h"
#include "p4ofagent/openflow-spec1.3.0.h"
#include "indigo/of_state_manager.h"

uint16_t
bit_mask_to_prefix (uint8_t *pv, int width) {
    uint16_t i;
    for (i = width * 8 - 1; i >= 0 && (*(pv + i / 8) & (1 << (i % 8))); i -= 1);
    return width * 8 - 1 - i;
}

/* ADD WRAPPERS */

//:: for table, t_info in table_info.items():
//::   if t_info["action_profile"] or table not in map_mod.openflow_tables:
//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
//::   of_match_fields = [i[1] for i in map_mod.openflow_tables[table].match_fields.items()]
//::   args_list = ["sess_hdl", "dev_tgt"]
//::   name = "_".join([table, "add"])
//::   pd_name = p4_pd_prefix + table + "_table_add_with_"
p4_pd_status_t
${name}
(
    of_match_t *match_fields,
    Pvoid_t *action_args,
    uint32_t sig,
    uint64_t flow_id,
    p4_pd_entry_hdl_t *entry_hdl,
    uint16_t *priority,
    uint32_t *ttl,
    p4_pd_sess_hdl_t sess_hdl,
    p4_pd_dev_target_t dev_tgt,
    uint8_t *signal
) {
//::   if match_params:
    ${p4_pd_prefix + table + "_match_spec_t match_spec"};
//::     args_list.append("&match_spec")
//::   #endif
//::   if t_info["match_type"] == "ternary":
//::     args_list.append("*priority")
//::   #endif
//::   args_list.append("&action_spec")
    ${p4_pd_prefix + "openflow_apply_action_spec_t action_spec"};
//::   if t_info["support_timeout"]:
//::     args_list.append("*ttl")
//::   #endif
//::   args_list += ["entry_hdl"]

    PWord_t pv;
    int rc;

//::   for fn, w in match_params:
//::     if fn in map_mod.openflow_tables[table].match_fields:
//::       val  = of_match_vals[map_mod.openflow_tables[table].match_fields[fn].field]
//::       mask = of_match_masks[map_mod.openflow_tables[table].match_fields[fn].field]
//::       fn_match_type = map_mod.openflow_tables[table].match_fields[fn].match_type
//::       if fn_match_type == "ternary":
//::         if w > 4:
    memcpy (match_spec.${fn + "_mask"}, ${mask}, ${w});
//::         else:
    match_spec.${fn + "_mask"} = ${mask};
//::         #endif
    if (${mask}) {
//::         if w > 4:
        memcpy (match_spec.${fn}, &${val}, ${w});
//::         else:
        match_spec.${fn} = ${val};
//::         #endif
    }
//::       elif fn_match_type == "lpm":
//::         if w > 4:
    memcpy (match_spec.${fn}, ${val}, ${w});
//::         else:
    match_spec.${fn} = ${val};
//::         #endif
    match_spec.${fn + "_prefix_length"} = bit_mask_to_prefix ((uint8_t *) &${mask}, ${w});
//::       else:
//::         if w > 4:
    memcpy (match_spec.${fn}, ${val}, ${w});
//::         else:
    match_spec.${fn} = ${val};
//::         #endif
//::       #endif
//::     #endif
//::   #endfor

//::   terns = [(fn, t) for fn, t in t_info["match_fields"] if t == "ternary"
//::             and fn not in map_mod.openflow_tables[table].match_fields]
//::   if terns:
//::     for fn, match_type in terns:
//::       for w in [i[1] for i in match_params if fn == i[0]]:
//::         if w > 4:
    match_spec.${fn + "_mask"} = { [0 ... ${w - 1}] = 0 };
//::         else:
    match_spec.${fn + "_mask"} = 0;
//::         #endif
//::       #endfor
//::     #endfor

//::   #endif
    action_spec.action_bmap = sig;

    if ((J1T (rc, *action_args, OFPAT_GROUP))) {
        JLG (pv, *action_args, OFPAT_GROUP);
        action_spec.action_group_id = *(uint32_t *) *pv;
    } else {
        action_spec.action_index = (uint32_t) flow_id;
    }

    if ((J1T (rc, *action_args, OFPAT_OUTPUT))) {
        JLG (pv, *action_args, OFPAT_OUTPUT);
        if (*(uint32_t *) *pv == OFPP_CONTROLLER) {
            ${p4_pd_prefix + "openflow_miss_action_spec_t miss_spec"};
            miss_spec.action_table_id = ${map_mod.openflow_tables[table].id};
            miss_spec.action_reason = OFPR_ACTION;
            *signal = 1;

            return ${pd_name}${"openflow_miss"}
                (${(", ").join(["&miss_spec" if i == "&action_spec" else i for i in args_list])});
        } else if (*(uint32_t *) *pv == OFPP_NORMAL) {
            return 0;
        }
    }

    return ${pd_name}${"openflow_apply"}
        (${(", ").join(args_list)});
}

//:: #endfor

/* MODIFY WRAPPERS */

//:: for table, t_info in table_info.items():
//::   if t_info["action_profile"] or table not in map_mod.openflow_tables:
//::     continue
//::   #endif
//::   args_list = ["sess_hdl", "dev_id", "entry_hdl"]
//::   name = "_".join([table, "mod"])
//::   pd_name = p4_pd_prefix + table + "_table_modify_with_"
p4_pd_status_t
${name}
(
    Pvoid_t *action_args,
    uint32_t sig,
    uint64_t flow_id,
    p4_pd_entry_hdl_t entry_hdl,
    p4_pd_sess_hdl_t sess_hdl,
    uint8_t dev_id,
    uint8_t *signal
) {
//::       args_list += ["&action_spec"]
    ${p4_pd_prefix + "openflow_apply_action_spec_t action_spec"};

    PWord_t pv;
    int rc;

    action_spec.action_bmap = sig;

    if ((J1T (rc, *action_args, OFPAT_GROUP))) {
        JLG (pv, *action_args, OFPAT_GROUP);
        action_spec.action_group_id = *(uint32_t *) *pv;
    } else {
        action_spec.action_index = (uint32_t) flow_id;
    }

    if ((J1T (rc, *action_args, OFPAT_OUTPUT))) {
        JLG (pv, *action_args, OFPAT_OUTPUT);
        if (*(uint32_t *) *pv == OFPP_CONTROLLER) {
            ${p4_pd_prefix + "openflow_miss_action_spec_t miss_spec"};
            miss_spec.action_table_id = ${map_mod.openflow_tables[table].id};
            miss_spec.action_reason = OFPR_ACTION;
            *signal = 1;

            return ${pd_name}${"openflow_miss"}
                (${(", ").join(["&miss_spec" if i == "&action_spec" else i for i in args_list])});
        } else if (*(uint32_t *) *pv == OFPP_NORMAL) {
            return 0;
        }
    }

    return ${pd_name}${"openflow_apply"}
        (${(", ").join(args_list)});
}

//::   #endfor

/* SET_DEFAULT WRAPPERS */

//:: for table, t_info in table_info.items():
//::   if t_info["action_profile"] or table not in map_mod.openflow_tables:
//::     continue
//::   #endif
//::   args_list = ["sess_hdl", "dev_tgt"]
//::   name = "_".join([table, "set_default"])
//::   pd_name = p4_pd_prefix + table + "_set_default_action_"
p4_pd_status_t
${name}
(
    Pvoid_t *action_args,
    uint32_t sig,
    uint64_t flow_id,
    p4_pd_entry_hdl_t *entry_hdl,
    p4_pd_sess_hdl_t sess_hdl,
    p4_pd_dev_target_t dev_tgt,
    uint8_t *signal
) {
//::       args_list.append("&action_spec")
    ${p4_pd_prefix + "openflow_apply_action_spec_t action_spec"};
//::       args_list += ["entry_hdl"]

    PWord_t pv;
    int rc;

    action_spec.action_bmap = sig;

    if ((J1T (rc, *action_args, OFPAT_GROUP))) {
        JLG (pv, *action_args, OFPAT_GROUP);
        action_spec.action_group_id = (uint32_t) *pv;
    } else {
        action_spec.action_index = (uint32_t) flow_id;
    }

    if ((J1T (rc, *action_args, OFPAT_OUTPUT))) {
        JLG (pv, *action_args, OFPAT_OUTPUT);
        if (*(uint32_t *) *pv == OFPP_CONTROLLER) {
            ${p4_pd_prefix + "openflow_miss_action_spec_t miss_spec"};
            miss_spec.action_table_id = ${map_mod.openflow_tables[table].id};
            miss_spec.action_reason = OFPR_ACTION;
            *signal = 1;

            return ${pd_name}${"openflow_miss"}
                (${(", ").join(["&miss_spec" if i == "&action_spec" else i for i in args_list])});
        } else if (*(uint32_t *) *pv == OFPP_NORMAL) {
            return 0;
        }
    }

    return ${pd_name}${"openflow_apply"}
        (${(", ").join(args_list)});
}

//:: #endfor
/* Initial settings for openflow module */ 

p4_pd_status_t
openflow_module_init () {
    uint8_t device = 0; // FIXME: device should be argument?
    uint16_t dev_pip_id = 0; // FIXME:
    p4_pd_status_t status = 0;
    p4_pd_entry_hdl_t entry_hdl;
    p4_pd_dev_target_t p4_pd_device;

    p4_pd_device.device_id = device;
    p4_pd_device.dev_pipe_id = dev_pip_id;

    // packet_out miss
    status = ${p4_pd_prefix}packet_out_set_default_action_nop (
        P4_PD_SESSION,
        p4_pd_device,
        &entry_hdl);

    // packet_out unicast
    ${p4_pd_prefix}packet_out_match_spec_t packet_out_ms;
    packet_out_ms.fabric_header_packetType = 5;
    packet_out_ms.fabric_header_cpu_reasonCode = 1;
    status |= ${p4_pd_prefix}packet_out_table_add_with_packet_out_unicast (
        P4_PD_SESSION,
        p4_pd_device,
        &packet_out_ms,
        &entry_hdl);

#define PRE_PORT_MAP_ARRAY_SIZE ((PRE_PORTS_MAX + 7)/8)

    // packet_out multicast
    packet_out_ms.fabric_header_packetType = 5;
    packet_out_ms.fabric_header_cpu_reasonCode = 2;

    static uint8_t port_list[PRE_PORT_MAP_ARRAY_SIZE];
    static uint8_t lag_map[PRE_PORT_MAP_ARRAY_SIZE];

    memset (port_list, 0xff, PRE_PORT_MAP_ARRAY_SIZE);
    memset (lag_map, 0, PRE_PORT_MAP_ARRAY_SIZE);

    // unset cpu port
    *(port_list + 64 / 8) ^= (1 << (64 % 8));

    mc_mgrp_hdl_t mgrp_hdl;
    mc_node_hdl_t node_hdl;

    mc_mgrp_create (P4_PRE_SESSION, 0, 20, &mgrp_hdl);
    mc_node_create (P4_PRE_SESSION, P4_DEVICE_ID, 0, port_list, lag_map, &node_hdl);
    mc_associate_node (P4_PRE_SESSION, mgrp_hdl, node_hdl);

    AGENT_ETHERNET_FLOOD_MC_HDL = mgrp_hdl;

    status |= ${p4_pd_prefix}packet_out_table_add_with_packet_out_eth_flood (
        P4_PD_SESSION,
        p4_pd_device,
        &packet_out_ms,
        &entry_hdl);

    // set pipeline defaults
    status |= ofpat_pipeline_set_default_nop ();

    return status;
}

void
openflow_init (Pvoid_t *adds, Pvoid_t *mods, Pvoid_t *defs,
               Pvoid_t *dels, Pvoid_t *read_bytes_hit,
               Pvoid_t *read_bytes_missed, Pvoid_t *read_packets_hit,
               Pvoid_t *read_packets_missed, Pvoid_t *per_flow_stats_bytes,
               Pvoid_t *per_flow_stats_packets) {
    PWord_t pv;
    indigo_core_table_ops_t *ops;

    openflow_module_init ();

//:: for table in map_mod.openflow_tables:
//::   table_id = map_mod.openflow_tables[table].id
    // ${table} state mods
    JLI (pv, *adds, ${table_id});
    *pv = (uint64_t) &${table + "_add"};
    JLI (pv, *mods, ${table_id});
    *pv = (uint64_t) &${table + "_mod"};
    JLI (pv, *defs, ${table_id});
    *pv = (uint64_t) &${table + "_set_default"};

    ops = malloc (sizeof (indigo_core_table_ops_t));

    ops->entry_create = &flow_create;
    ops->entry_modify = &flow_modify;
    ops->entry_delete = &flow_delete;
    ops->table_stats_get = &table_stats_get;

    indigo_core_table_register (${table_id}, "${table}", ops,
                                (void *) (uint64_t) ${table_id});

    // Global ${table} counters
    JLI (pv, *dels, ${table_id});
    *pv = (uint64_t) &${p4_pd_prefix + table + "_table_delete"};
    JLI (pv, *read_bytes_hit, ${table_id});
    *pv = (uint64_t) &${p4_pd_prefix + table + "_table_read_bytes_counter_hit"};
    JLI (pv, *read_bytes_missed, ${table_id});
    *pv = (uint64_t) &${p4_pd_prefix + table + "_table_read_bytes_counter_miss"};
    JLI (pv, *read_packets_hit, ${table_id});
    *pv = (uint64_t) &${p4_pd_prefix + table + "_table_read_packets_counter_hit"};
    JLI (pv, *read_packets_missed, ${table_id});
    *pv = (uint64_t) &${p4_pd_prefix + table + "_table_read_packets_counter_miss"};

//:: #endfor
    // per flow counters
//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct" and binding[1] in map_mod.openflow_tables:
//::     name = "p4_pd_" + p4_prefix + "_counter_read_" + counter
    JLI (pv, *per_flow_stats_${type_}, ${map_mod.openflow_tables[binding[1]].id});
    *pv = (uint64_t) &${name};

//::   #endif
//:: #endfor

    // Group table
    indigo_core_group_table_ops_t *group_ops;
    group_ops = malloc (sizeof (indigo_core_group_table_ops_t));
    group_ops->entry_create = &group_create;
    group_ops->entry_modify = &group_modify;
    group_ops->entry_delete = &group_delete;
    group_ops->entry_stats_get = &group_stats;

    indigo_core_group_table_register
        (${len(map_mod.openflow_tables)}, "${p4_pd_prefix + "group"}", group_ops,
         (void *) (uint64_t) ${len(map_mod.openflow_tables)});
}

uint8_t
num_openflow_tables () {
    return ${len(map_mod.openflow_tables)};
}
