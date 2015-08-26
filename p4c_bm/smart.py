# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import p4_hlir.hlir.p4 as p4
from tenjin.util import *
import os
import globals as gl
import re
import socket
from collections import defaultdict, OrderedDict
from util.topo_sorting import Graph, Node
from time import sleep
import sys
import json
from pkg_resources import resource_string

_SMART_DIR = os.path.dirname(os.path.realpath(__file__))

templates_dir = os.path.join(_SMART_DIR, "templates")
plugin_base = os.path.join(_SMART_DIR, 'plugin/')

re_brackets = re.compile('[\[\]]')

def bytes_round_up(bits):
    return (bits + 7) / 8

def get_header_instance_name(p4_header_instance):
    header_name = p4_header_instance.name
    # cannot have brackets in the header name in C
    header_name = re_brackets.sub("_", header_name)
    return header_name

def get_field_instance_name(p4_field_instance, header_name = None):
    if header_name is None:
        header_name = get_header_instance_name(p4_field_instance.instance)
    # return "%s_%s" % (header_name, p4_field_instance.name)
    return "%s_%s" % (header_name, p4_field_instance.name)

def get_action_name(p4_action):
    return p4_action.name

def get_table_name(p4_table):
    return p4_table.name

def get_conditional_node_name(p4_conditional_node):
    return p4_conditional_node.name

def int_to_byte_array(val, num_bytes):
    res = []
    for i in xrange(num_bytes):
        res.append(int(val % 256))
        val /= 256
    res.reverse()
    return res

def build_switch_match_key(field_widths, value):
    res = []
    for width in reversed(field_widths):
        mask = (1 << width) - 1
        val = value & mask
        num_bytes = max(bytes_round_up(width), 4)
        res = int_to_byte_array(val, num_bytes) + res
        value = value >> width
    return res

def produce_parser_topo_sorting(hlir):
    header_graph = Graph()

    def walk_rec(hlir, parse_state, prev_hdr_node, tag_stacks_index):
        assert(isinstance(parse_state, p4.p4_parse_state))
        for call in parse_state.call_sequence:
            call_type = call[0]
            if call_type == p4.parse_call.extract:
                hdr = call[1]

                if hdr.virtual:
                    base_name = hdr.base_name
                    current_index = tag_stacks_index[base_name]
                    if current_index > hdr.max_index:
                        return
                    tag_stacks_index[base_name] += 1
                    name = base_name + "[%d]" % current_index
                    hdr = hlir.p4_header_instances[name]

                if hdr not in header_graph:
                    header_graph.add_node(hdr)
                hdr_node = header_graph.get_node(hdr)

                if prev_hdr_node:
                    prev_hdr_node.add_edge_to(hdr_node)
                else:
                    header_graph.root = hdr;
                prev_hdr_node = hdr_node

        for branch_case, next_state in parse_state.branch_to.items():
            if not next_state: continue
            if not isinstance(next_state, p4.p4_parse_state):
                continue
            walk_rec(hlir, next_state, prev_hdr_node, tag_stacks_index.copy())


    start_state = hlir.p4_parse_states["start"]
    walk_rec(hlir, start_state, None, defaultdict(int))

    header_topo_sorting = header_graph.produce_topo_sorting()

    # topo_sorting = []
    # for hdr in header_topo_sorting:
    #     topo_sorting.append(get_header_instance_name(hdr))

    # dirty fix, to ensure that all tags in a stag are contiguous
    # this makes the (questionable) assumption that all those tags are
    # contiguous in the deparsed packet
    buckets = defaultdict(list)
    for header_instance in header_topo_sorting:
        base_name = header_instance.base_name
        buckets[base_name].append(header_instance)
    collapsed = []
    for header_instance in header_topo_sorting:
        base_name = header_instance.base_name
        collapsed += buckets[base_name]
        buckets[base_name] = []

    return collapsed

def render_dict_populate_fields(render_dict, hlir, meta_config_json):
    render_dict["ordered_field_instances_all"] = []
    render_dict["ordered_field_instances_virtual"] = []
    render_dict["ordered_field_instances_non_virtual"] = []
    render_dict["field_info"] = {}

    render_dict["ordered_header_instances_all"] = []
    render_dict["ordered_header_instances_virtual"] = []
    render_dict["ordered_header_instances_non_virtual"] = []
    render_dict["tag_stacks_depth"] = defaultdict(int)
    render_dict["tag_stacks_first"] = {}
    render_dict["header_info"] = {}

    phv_offset = 0

    # TODO: improve
    headers_regular = produce_parser_topo_sorting(hlir)
    base_names_regular = set()
    for header_instance in headers_regular:
        base_names_regular.add(header_instance.base_name)
    headers_metadata = []
    headers_virtual = []
    headers_non_virtual = []
    for header_instance in hlir.p4_header_instances.values():
        if header_instance.virtual:
            # TODO: temp fix, do something better
            if header_instance.base_name not in base_names_regular:
                continue
            headers_virtual.append(header_instance)
        elif header_instance.metadata:
            headers_metadata.append(header_instance)

    headers_non_virtual = headers_regular + headers_metadata
    headers_ordered = headers_virtual + headers_non_virtual

    get_name = lambda x: get_header_instance_name(x)
    render_dict["ordered_header_instances_all"] = map(get_name, headers_ordered)
    render_dict["ordered_header_instances_virtual"] = map(get_name, headers_virtual)
    render_dict["ordered_header_instances_non_virtual"] = map(get_name, headers_non_virtual)
    render_dict["ordered_header_instances_metadata"] = map(get_name, headers_metadata)
    render_dict["ordered_header_instances_regular"] = map(get_name, headers_regular)

    for header_instance in headers_ordered:
        header_name = get_header_instance_name(header_instance)

        base_name = header_instance.base_name
        h_info = {}
        virtual = header_instance.virtual

        h_info["base_name"] = base_name
        h_info["virtual"] = virtual
        h_info["is_metadata"] = header_instance.metadata

        if virtual:
            if header_instance.index == p4.P4_NEXT:
                type_ = "next"
            elif header_instance.index == p4.P4_LAST:
                type_ = "last"
            else:
                assert(False)
            h_info["virtual_type"] = type_
        else:
            h_info["byte_offset_phv"] = phv_offset

            if header_instance.index is not None:
                # assume they are ordered
                if base_name not in render_dict["tag_stacks_first"]:
                    render_dict["tag_stacks_first"][base_name] = header_name
                render_dict["tag_stacks_depth"][base_name] += 1

        h_bit_width = 0
        h_byte_width_phv = 0
        h_info["fields"] = []

        field_index = 0
        for field_instance in header_instance.fields:
            field_name = get_field_instance_name(field_instance, header_name)

            render_dict["ordered_field_instances_all"].append(field_name)

            if virtual:
                render_dict["ordered_field_instances_virtual"].append(field_name)
            else:
                render_dict["ordered_field_instances_non_virtual"].append(field_name)

            f_info = {}
            bit_width = field_instance.width
            h_bit_width += bit_width

            f_info["field_index"] = field_index
            field_index += 1
            f_info["virtual"] = virtual
            f_info["bit_width"] = bit_width
            f_info["bit_offset_hdr"] = field_instance.offset % 8
            f_info["byte_offset_hdr"] = field_instance.offset / 8
            f_info["parent_header"] = header_name
            f_info["is_metadata"] = h_info["is_metadata"]

            # only 2 data types: uint32_t and byte_buf_t
            if bit_width <= 32:
                f_info["data_type"] = "uint32_t"
                f_info["byte_width_phv"] = 4
                f_info["mask"] = hex(int(socket.htonl((1 << bit_width) - 1)))
            else:
                # TODO
                assert(bit_width % 8 == 0)
                f_info["data_type"] = "byte_buf_t"
                f_info["byte_width_phv"] = bit_width / 8
                f_info["mask"] = hex(0)

            if f_info["is_metadata"]:
                f_info["default"] = int_to_byte_array(field_instance.default,
                                                      f_info["byte_width_phv"])

            h_byte_width_phv += f_info["byte_width_phv"]

            if not virtual:
                f_info["byte_offset_phv"] = phv_offset
                phv_offset += f_info["byte_width_phv"]

            render_dict["field_info"][field_name] = f_info

            h_info["fields"].append(field_name)

        h_info["bit_width"] = h_bit_width
        h_info["byte_width_phv"] = h_byte_width_phv

        render_dict["header_info"][header_name] = h_info

    # total length (in bytes) of the phv memory block. Will be allocated
    # dynamically (once) at run time
    phv_length = phv_offset
    render_dict["phv_length"] = phv_length
    render_dict["num_headers"] = len(render_dict["ordered_field_instances_non_virtual"])

    print "total phv length (in bytes):", phv_length

    # checksums
    cond_idx = 1
    calculated_fields_verify = OrderedDict()
    calculated_fields_update = OrderedDict()
    calculated_fields_conds = []
    for header_instance in headers_regular:
        for field_instance in header_instance.fields:
            field_name = get_field_instance_name(field_instance)
            calculated_fields_verify[field_name] = []
            calculated_fields_update[field_name] = []
            for calculation in field_instance.calculation:
                type_, calc, if_cond = calculation
                assert(calc.output_width == field_instance.width)
                if if_cond:
                    if_cond = dump_p4_expression(if_cond)
                    calculated_fields_conds.append(if_cond)
                    if_cond_idx = cond_idx
                    cond_idx += 1
                else:
                    if_cond_idx = 0
                if type_ == "verify":
                    calculated_fields_verify[field_name] += [(calc.name, if_cond_idx)]
                if type_ == "update":
                    calculated_fields_update[field_name] += [(calc.name, if_cond_idx)]
    render_dict["calculated_fields_conds"] = calculated_fields_conds
    render_dict["calculated_fields_verify"] = calculated_fields_verify
    render_dict["calculated_fields_update"] = calculated_fields_update


    config = meta_config_json

    # This is the set of data carried for recirculation
    render_dict["meta_carry"] = config["meta_carry"]

    # This maps metadata short names to full references
    metadata_name_map = config["meta_mappings"]["standard_metadata"]
    intrinsic_metadata_name_map = config["meta_mappings"]["intrinsic_metadata"]
    pre_metadata_name_map = config["meta_mappings"]["pre_metadata"]
    render_dict["metadata_name_map"] = metadata_name_map
    render_dict["intrinsic_metadata_name_map"] = intrinsic_metadata_name_map
    render_dict["pre_metadata_name_map"] = pre_metadata_name_map

    render_dict["extra_metadata_name_map"] = {}
    if "extra_metadata" in config["meta_mappings"]:
        render_dict["extra_metadata_name_map"] = config["meta_mappings"]["extra_metadata"]

    # Calculate offsets for metadata entries
    offset = 0
    metadata_offsets = {}
    num_intrinsic_metadata = 0
    num_pre_metadata = 0
    for header_instance in map(get_name, headers_non_virtual):
        h_info = render_dict["header_info"][header_instance]
        if h_info["is_metadata"]:
            for field in h_info["fields"]:
                if field in metadata_name_map.values():
                    metadata_offsets[field] = offset
                elif field in intrinsic_metadata_name_map.values():
                    num_intrinsic_metadata += 1
                    metadata_offsets[field] = offset
                elif field in pre_metadata_name_map.values():
                    num_pre_metadata += 1
                    metadata_offsets[field] = offset
                offset += render_dict["field_info"][field]["byte_width_phv"]
    enable_intrinsic = 1 if len(intrinsic_metadata_name_map) == num_intrinsic_metadata else 0
    enable_pre = 1 if len(pre_metadata_name_map) == num_pre_metadata else 0
    render_dict["metadata_offsets"] = metadata_offsets
    render_dict["enable_intrinsic"] = enable_intrinsic
    render_dict["enable_pre"] = enable_pre


def render_dict_populate_data_types(render_dict, hlir):
    render_dict["field_data_types"] = ["uint32_t", "byte_buf_t"]

def render_dict_populate_parse_states(render_dict, hlir):
    render_dict["parse_states"] = OrderedDict()
    render_dict["value_sets"] = OrderedDict()

    header_info = render_dict["header_info"]
    field_info = render_dict["field_info"]

    value_sets_used = set()

    for name, parse_state in hlir.p4_parse_states.items():
        parse_info = {}

        call_sequence = []
        for call in parse_state.call_sequence:
            call_type = call[0]
            if call_type == p4.parse_call.extract:
                extract_name = get_header_instance_name(call[1])
                call_sequence += [(str(call_type), extract_name)]
            elif call_type == p4.parse_call.set:
                metadata_name = get_field_instance_name(call[1])

                target_byte_width = field_info[metadata_name]["byte_width_phv"]

                metadata_value = call[2]

                if type(metadata_value) is p4.p4_field:
                    call_sequence += [(str(call_type),
                                       metadata_name,
                                       "latest",
                                       get_field_instance_name(metadata_value))]

                elif type(metadata_value) is int:
                    value = call[2]
                    byte_array = int_to_byte_array(value, target_byte_width)
                    call_sequence += [(str(call_type),
                                       metadata_name,
                                       "immediate",
                                       byte_array)]

                else:
                    # tuple, for current
                    call_sequence += [(str(call_type),
                                       metadata_name,
                                       "current",
                                       metadata_value)]

            else: # counters
                # TODO
                assert(False)
        parse_info["call_sequence"] = call_sequence

        branch_on = []
        key_fields_bit_widths = []
        key_byte_width = 0
        for switch_ref in parse_state.branch_on:
            if type(switch_ref) is p4.p4_field:
                field_ref = switch_ref
                field_instance_name = get_field_instance_name(field_ref)
                field_bit_width = field_info[field_instance_name]["bit_width"]
                key_fields_bit_widths.append(field_bit_width)
                num_bytes = max(bytes_round_up(field_bit_width), 4)
                branch_on.append(("field_ref", field_instance_name))
            elif type(switch_ref) is tuple:
                offset, bit_width = switch_ref
                key_fields_bit_widths.append(bit_width)
                num_bytes = max(bytes_round_up(bit_width), 4)
                branch_on.append(("current", switch_ref))
            else:
                assert(False)
            key_byte_width += num_bytes
        parse_info["branch_on"] = branch_on

        value_set_info = {}

        branch_to = []
        for branch_case, next_state in parse_state.branch_to.items():
            value, mask = 0, 0;
            if branch_case == p4.P4_DEFAULT:
                case_type = "default"
            elif type(branch_case) is p4.p4_parse_value_set:
                if branch_case in value_sets_used:
                    print "a value set can only be used in one parse state"
                    assert(False)
                value_set_info["match_data"] = branch_on
                value_set_info["byte_width"] = key_byte_width
                render_dict["value_sets"][branch_case.name] = value_set_info
                value_sets_used.add(branch_case)

                case_type = "value_set"
                value = branch_case.name
            elif type(branch_case) is int:
                case_type = "value"
                value = build_switch_match_key(key_fields_bit_widths,
                                               branch_case)
                # print hex(branch_case), key_fields_bit_widths, value
            else:
                case_type = "value_masked"
                value = build_switch_match_key(key_fields_bit_widths,
                                               branch_case[0])
                mask = build_switch_match_key(key_fields_bit_widths,
                                              branch_case[1])

            if next_state is None:
                # should not happen any more
                assert(False)
                next_parse_state = ("parse_state", "end")
            if isinstance(next_state, p4.p4_parse_state):
                next_parse_state = ("parse_state", next_state.name)
            elif isinstance(next_state, p4.p4_conditional_node):
                next_parse_state = ("conditional_table",
                                    get_conditional_node_name(next_state))
            elif isinstance(next_state, p4.p4_table):
                next_parse_state = ("table",
                                    get_table_name(next_state))
            else:
                assert(False)

            branch_to += [(case_type, value, mask, next_parse_state)]
        parse_info["branch_to"] = branch_to

        render_dict["parse_states"][name] = parse_info

def render_dict_populate_actions(render_dict, hlir):
    render_dict["action_info"] = {}

    field_info = render_dict["field_info"]

    # these are the only actions for which we have to generate C code, since we
    # are using the flat sequence of primitive calls
    table_actions_set = set()
    for _, table in hlir.p4_tables.items():
        for action in table.actions: table_actions_set.add(action)

    # we need to get the size of the match data
    for action in table_actions_set:
        a_info = {}

        param_names = []
        param_byte_widths = []
        param_bit_widths = []
        for param, width in zip(action.signature, action.signature_widths):
            if not width                 :
                print "unused parameter discarded"
                continue
            param_names += [param]
            param_byte_widths += [max(bytes_round_up(width), 4)]
            param_bit_widths += [width]
        a_info["param_names"] = param_names
        a_info["param_byte_widths"] = param_byte_widths
        a_info["param_bit_widths"] = param_bit_widths

        # find out which fields / instances we need to copy for parallel
        # execution semantics
        field_access = defaultdict(set)
        for call in action.flat_call_sequence:
            primitive = call[0]
            for index, arg in enumerate(call[1]):
                if type(arg) is p4.p4_field or\
                   type(arg) is p4.p4_header_instance:
                    sig_arg_name = primitive.signature[index]
                    flags = primitive.signature_flags[sig_arg_name]
                    if "access" not in flags:
                        field_access[arg].add(p4.P4_WRITE)
                        field_access[arg].add(p4.P4_READ)
                    else:
                        access = flags["access"]
                        field_access[arg].add(access)
        field_copies = set()
        header_copies = set()
        for arg, access in field_access.items():
            if len(access) > 1: # read and write
                if type(arg) is p4.p4_field:
                    field_copies.add(get_field_instance_name(arg))
                else:
                    header_copies.add(get_header_instance_name(arg))

        a_info["field_copies"] = field_copies
        a_info["header_copies"] = header_copies

        call_sequence = []
        for call in action.flat_call_sequence:
            primitive_name = get_action_name(call[0]).upper()
            primitive_args = []

            # is this outdated, do I really need to have the immedaite value be
            # the exact same size as the destination? Let's try to get rid of
            # it.

            # width = 0
            # if call[0].name == "add_to_field" or\
            #    call[0].name == "modify_field" or\
            #    call[0].name == "modify_field_with_hash_based_offset":
            #     ref_arg = call[1][0]
            #     field_name = get_field_instance_name(ref_arg)
            #     width = field_info[field_name]["byte_width_phv"]

            for arg in call[1]:
                if type(arg) is int or type(arg) is long:
                    # assert(width > 0)
                    tmp = arg
                    nbytes = 0
                    while tmp > 0:
                        nbytes += 1
                        tmp /= 256
                    width = max(4, nbytes)
                    type_ = "immediate"
                    value = int_to_byte_array(arg, width)
                elif type(arg) is p4.p4_field:
                    type_ = "field_ref"
                    value = get_field_instance_name(arg)
                elif type(arg) is p4.p4_header_instance:
                    type_ = "header_ref"
                    value = get_header_instance_name(arg)
                elif type(arg) is p4.p4_signature_ref:
                    type_ = "param"
                    value = arg.idx
                elif type(arg) is p4.p4_parse_state:
                    type_ = "parse_state"
                    value = arg.name
                elif type(arg) is p4.p4_field_list:
                    type_ = "field_list"
                    value = arg.name
                elif type(arg) is p4.p4_field_list_calculation:
                    type_ = "p4_field_calculation"
                    value = arg.name
                elif type(arg) is p4.p4_counter:
                    type_ = "p4_counter"
                    value = arg.name
                elif type(arg) is p4.p4_meter:
                    type_ = "p4_meter"
                    value = arg.name
                elif type(arg) is p4.p4_register:
                    type_ = "p4_register"
                    value = arg.name
                else:
                    print type(arg)
                    assert(False)

                primitive_args.append((type_, value))

            call_sequence.append((primitive_name, primitive_args))

        a_info["call_sequence"] = call_sequence

        action_name = get_action_name(action)
        render_dict["action_info"][action_name] = a_info

def render_dict_populate_conditional_tables(render_dict, hlir):
    render_dict["conditional_table_info"] = OrderedDict()

    for name, cnode in hlir.p4_conditional_nodes.items():
        ct_info = {}

        ct_info["expression"] = str(cnode.condition)

        conditional_name = get_conditional_node_name(cnode)
        ct_info["expression_computation"] = dump_p4_expression(cnode.condition)

        ct_info["next_tables"] = {}  # True, False
        for b, next_ in cnode.next_.items():
            if next_ is None:
                next_name = None
            if isinstance(next_, p4.p4_conditional_node):
                next_name = get_conditional_node_name(next_)
            elif isinstance(next_, p4.p4_table):
                next_name = get_table_name(next_)
            ct_info["next_tables"][b] = next_name

        render_dict["conditional_table_info"][conditional_name] = ct_info

def render_dict_populate_tables(render_dict, hlir):
    render_dict["table_info"] = OrderedDict()

    field_info = render_dict["field_info"]
    action_info = render_dict["action_info"]

    select_tables = []
    action_data_tables = []

    for name, table in hlir.p4_tables.items():
        t_info = {}

        # can be None
        t_info["min_size"] = table.min_size
        t_info["max_size"] = table.max_size
        t_info["support_timeout"] = table.support_timeout
        t_info["action_profile"] = None
        act_prof = table.action_profile
        if act_prof is not None:
            t_info["action_profile"] = act_prof.name
            action_data_tables.append(name)
            if act_prof.selector is not None:
                select_tables.append(name)

        # will be set in render_dict_populate_counters
        t_info["bytes_counter"] = None
        t_info["packets_counter"] = None
        t_info["meter"] = None
        t_info["registers"] = []

        match_types = []
        match_precedence = {
            p4.p4_match_type.P4_MATCH_VALID: 0,
            p4.p4_match_type.P4_MATCH_EXACT: 1,
            p4.p4_match_type.P4_MATCH_TERNARY: 3,
            p4.p4_match_type.P4_MATCH_LPM: 2,
        }
        for _, m_type, _ in table.match_fields:
            if m_type not in match_precedence:
                print m_type, "match not yet supported"
                assert(False)
            match_types.append(m_type)

        # If no match fields, indicate exact match
        if len(match_types) == 0:
            match_type = p4.p4_match_type.P4_MATCH_EXACT
        elif p4.p4_match_type.P4_MATCH_TERNARY in match_types:
            match_type = p4.p4_match_type.P4_MATCH_TERNARY
        elif match_types.count(p4.p4_match_type.P4_MATCH_LPM) >= 2:
            print "cannot have 2 different lpm in a single table"
            assert(False)
        elif p4.p4_match_type.P4_MATCH_LPM in match_types:
            match_type = p4.p4_match_type.P4_MATCH_LPM
        else:
            # that includes the case when we only have one valid match and
            # nothing else
            match_type = p4.p4_match_type.P4_MATCH_EXACT

        type_mappings = {
            p4.p4_match_type.P4_MATCH_EXACT: "exact",
            p4.p4_match_type.P4_MATCH_TERNARY: "ternary",
            p4.p4_match_type.P4_MATCH_LPM: "lpm",
            p4.p4_match_type.P4_MATCH_RANGE: "range",
            p4.p4_match_type.P4_MATCH_VALID: "valid",
            None: "none",
        }

        t_info["match_type"] = type_mappings[match_type]

        # basically same code as for branch_on in parse functions, because the
        # build_key function is going to be the same
        match_fields = []
        key_fields_bit_widths = []
        big_mask = []
        has_mask = False
        key_byte_width = 0
        reordered_fields_idx = sorted(
            range(len(table.match_fields)),
            key = lambda x: match_precedence[table.match_fields[x][1]]
        )
        for field_ref, m_type, mask in table.match_fields:
            if m_type is p4.p4_match_type.P4_MATCH_VALID:
                if type(field_ref) is p4.p4_header_instance:
                    header_ref = field_ref
                elif type(field_ref) is p4.p4_field:
                    header_ref = field_ref.instance
                else:
                    assert(False) # should not happen
                header_instance_name = get_header_instance_name(header_ref)
                field_bit_width = 1
                key_fields_bit_widths.append(field_bit_width)
                num_bytes = 1
                key_byte_width += num_bytes # this will use only 1 byte, not 4
                match_fields += [(header_instance_name, type_mappings[m_type])]

            else:
                field_instance_name = get_field_instance_name(field_ref)
                field_bit_width = field_info[field_instance_name]["bit_width"]
                key_fields_bit_widths.append(field_bit_width)
                num_bytes = max(bytes_round_up(field_bit_width), 4)
                key_byte_width += num_bytes
                match_fields += [(field_instance_name, type_mappings[m_type])]

            # common to all match types
            if mask:
                big_mask += int_to_byte_array(mask, num_bytes)
                has_mask = True
            else:
                big_mask += [255 for i in xrange(num_bytes)]

        t_info["match_fields"] = match_fields
        t_info["reordered_match_fields_idx"] = reordered_fields_idx
        t_info["reordered_match_fields"] = [match_fields[i] \
                                            for i in reordered_fields_idx]
        t_info["has_mask"] = has_mask
        t_info["big_mask"] = big_mask
        # will be useful for PD code
        t_info["key_fields_bit_widths"] = key_fields_bit_widths
        t_info["key_byte_width"] = key_byte_width

        t_info["actions"] = [get_action_name(a) for a in table.actions]

        t_info["next_tables"] = {}
        with_hit_miss_spec = False
        if "hit" in table.next_:
            with_hit_miss_spec = True
        t_info["with_hit_miss_spec"] = with_hit_miss_spec
        if with_hit_miss_spec:
            for event in {"hit", "miss"}:
                t = table.next_[event]
                if t:
                    t_info["next_tables"][event] = get_table_name(t)
                else:
                    t_info["next_tables"][event] = None
        else:
            for a in table.actions:
                t = table.next_[a]
                if t:
                    t_info["next_tables"][get_action_name(a)] = get_table_name(t)
                else:
                    t_info["next_tables"][get_action_name(a)] = None

        actions_idx = {}
        idx = 0
        for action in t_info["actions"]:
            actions_idx[action] = idx
            idx += 1
        t_info["actions_idx"] = actions_idx

        if table.action_profile is None:
            action_data_byte_width = 0
            for action in t_info["actions"]:
                a_info = action_info[action]
                action_data_byte_width = max(action_data_byte_width,
                                             sum(a_info["param_byte_widths"]))
            t_info["action_data_byte_width"] = action_data_byte_width
        else:
            # with an action profile, the first bit is for group vs.member, the
            # remaining 31 bits are for index value
            t_info["action_data_byte_width"] = 4

        table_name = get_table_name(table)

        render_dict["table_info"][table_name] = t_info

    render_dict["select_tables"] = select_tables
    render_dict["action_data_tables"] = action_data_tables

def render_dict_populate_table_types(render_dict, hlir):
    render_dict["table_types"] = ["lpm", "exact", "ternary", "range", "valid",
                                  "none"]

def render_dict_populate_action_profiles(render_dict, hlir):
    action_profiles = OrderedDict()

    action_info = render_dict["action_info"]

    for name, act_prof in hlir.p4_action_profiles.items():
        act_prof_info = {}

        act_prof_info["actions"] = [get_action_name(a) for a in act_prof.actions]

        # actions_idx = {}
        # idx = 0
        # for action in act_prof_info["actions"]:
        #     actions_idx[action] = idx
        #     idx += 1
        # act_prof_info["actions_idx"] = actions_idx
        action_data_byte_width = 0
        for action in act_prof_info["actions"]:
            a_info = action_info[action]
            action_data_byte_width = max(action_data_byte_width,
                                         sum(a_info["param_byte_widths"]))
        act_prof_info["action_data_byte_width"] = action_data_byte_width

        act_prof_info["size"] = act_prof.size

        act_prof_info["selection_key"] = None
        if act_prof.selector is not None:
            act_prof_info["selection_key"] = act_prof.selector.selection_key.name

        action_profiles[name] = act_prof_info

    render_dict["action_profiles"] = action_profiles

def dump_p4_expression(expression):
    # This function generates a regiter name for an expression to store its
    # result.
    def get_next_register():
        register_name = "reg[" + str(get_next_register.register_counter) + "]"
        get_next_register.register_counter += 1
        return register_name
    get_next_register.register_counter = 0

    # Assignment statements used to generate C code are stored in this array.
    # An assignment statement is a tuple of the following format:
    # (register_name, operator, operand1, optional operand2, ...)
    register_assignments = []
    dump_register_assignments(expression, get_next_register, register_assignments)
    return register_assignments


# Only supports expressions involving uint32_t fields
def dump_register_assignments(expression, get_next_register, register_assignments):
    if expression is None: return None

    # The result of an expression should be stored in the first register. While
    # generating C code, we are assuming that the top-level expression stores
    # its result in reg[0].
    register = get_next_register()

    if type(expression) is int:
        register_assignments.append((register, "assign_immediate", expression))
    elif type(expression) is p4.p4_header_instance:
        assert(False)
    elif type(expression) is p4.p4_field:
        assert(expression.width <= 32)
        register_assignments.append((register, "assign_field", get_field_instance_name(expression)))
    elif type(expression) is p4.p4_expression:
        left = expression.left
        right = expression.right
        op = expression.op
        if op == "not":
            operand_register = dump_register_assignments(right, get_next_register, register_assignments)
            register_assignments.append((register, "not", operand_register))
        elif op in {"or", "and", "==", "!=", ">", ">=", "<", "<=", "&"}:
            left_register = dump_register_assignments(left, get_next_register, register_assignments)
            right_register = dump_register_assignments(right, get_next_register, register_assignments)
            register_assignments.append((register, op, left_register, right_register))
        elif op == "valid":
            if type(right) is p4.p4_header_instance:
                register_assignments.append((register, "valid_header", get_header_instance_name(right)))
            elif type(right) is p4.p4_field:
                register_assignments((register, "valid_field", get_field_instance_name(right)))
            else:
                assert(False)
        else:
            print "operation", op, "is not supported by the behavioral model"
            assert(False)
    else:
        print "Expression ", expression, "of type", type(expression), "not supported"
        assert(False)

    return register


def render_dict_populate_field_lists(render_dict, hlir):
    field_lists = {}
    for name, field_list in hlir.p4_field_lists.items():
        f_list = []
        prev_header = None # for payload, to make my life easier
        for field in field_list.fields:
            if type(field) is int or type(field) is long:
                print "values not supported in field lists yet"
                assert(False)
                type_ = "immediate"
                value = int_to_byte_array(field, width)
            elif type(field) is p4.p4_field:
                type_ = "field_ref"
                value = get_field_instance_name(field)
                prev_header = field.instance
            elif type(field) is p4.p4_header_instance:
                type_ = "header_ref"
                value = get_header_instance_name(field)
                prev_header = field
            elif type(field) is p4.p4_field_list:
                print "recursive field lists not supported yet"
                assert(False)
            elif field is p4.P4_PAYLOAD:
                type_ = "payload"
                value = get_header_instance_name(prev_header)
            elif type(field) is p4.p4_sized_integer:
                assert(field.width % 8 == 0)
                type_ = "integer"
                value = int_to_byte_array(field, field.width / 8)
            f_list += [(type_, value)]
        field_lists[name] = f_list

    p4_pd_prefix = render_dict["p4_pd_prefix"]
    learn_quanta = []
    field_list_set = set()
    for _, table in hlir.p4_tables.items():
        for action in table.actions:
            for call in action.flat_call_sequence:
                primitive = call[0]
                if primitive.name == "generate_digest":
                    for arg in call[1]:
                        if type(arg) is p4.p4_field_list:
                            field_list_object = arg

                            field_instances = OrderedDict() 
                            for field_instance in [x[1] for x in field_lists[field_list_object.name]]:
                                # This is a map. The value is always None.
                                field_instances[field_instance] = None

                            prefix = p4_pd_prefix + field_list_object.name
                            lf_prefix = "lf_" + field_list_object.name
                            lq = {"name" : field_list_object.name,
                                  "entry_type" : prefix + "_digest_entry_t",
                                  "msg_type" : prefix + "_digest_msg_t",
                                  "cb_fn_type" : prefix + "_digest_notify_cb",
                                  "register_fn" : prefix + "_register",
                                  "deregister_fn" : prefix + "_deregister",
                                  "notify_ack_fn" : prefix + "_notify_ack",
                                  "lf_register_fn" : lf_prefix + "_register",
                                  "lf_deregister_fn" : lf_prefix + "_deregister",
                                  "lf_notify_ack_fn" : lf_prefix + "_notify_ack",
                                  "fields" : field_instances}
                            if field_list_object.name not in field_list_set:
                                learn_quanta.append(lq)
                                field_list_set.add(field_list_object.name)

    render_dict["field_lists"] = field_lists
    render_dict["learn_quanta"] = learn_quanta


def render_dict_populate_field_list_calculations(render_dict, hlir):
    field_list_calculations = {}

    def get_size(field, size = 0):
        if type(field) is int or type(field) is long:
            print "values not supported in field lists yet"
            assert(False)
        elif type(field) is p4.p4_sized_integer:
            assert(field.width % 8 == 0)
            return size + field.width
        elif type(field) is p4.p4_field:
            return size + field.width
        elif type(field) is p4.p4_header_instance:
            assert(size % 8 == 0)
            for subfield in field.fields:
                size = get_size(subfield, size)
            return size
        elif type(field) is p4.p4_field_list:
            for subfield in field.fields:
                size = get_size(subfield, size)
            return size
        elif field is p4.P4_PAYLOAD:
            # TODO: ...
            return size + 2048 * 8
        else:
             assert(False)

    for name, calculation in\
        hlir.p4_field_list_calculations.items():
        c_info = {}
        c_info["input"] = [fl.name for fl in calculation.input]
        assert(len(c_info["input"]) == 1)
        c_info["input_sizes"] = []
        for input_ in calculation.input:
            bits = get_size(input_)
            assert(bits % 8 == 0)
            c_info["input_sizes"] += [bits / 8]
        c_info["input_size"] = max(c_info["input_sizes"])
        # assert(len(calculation.algorithms) == 1)
        # c_info["algorithm"] = calculation.algorithms[0]
        c_info["algorithm"] = calculation.algorithm
        # TODO: define exactly what I want to do here
        c_info["output_width"] = calculation.output_width
        if calculation.output_width > 32:
            assert(calculation.output_width % 8 == 0)
        c_info["output_phv_bytes"] = max(bytes_round_up(calculation.output_width), 4)
        field_list_calculations[name] = c_info

    render_dict["field_list_calculations"] = field_list_calculations


def render_dict_populate_counters(render_dict, hlir):
    counter_info = {}
    for name, counter in hlir.p4_counters.items():
        type_ = str(counter.type)

        if type_ == "packets" or type_ == "bytes":
            c_list = [(name, type_)]
        elif type_ == "packets_and_bytes":
            c_list = [(name + "_bytes", "bytes"), (name + "_packets", "packets")]
        else:
            assert(False)

        for name, type_ in c_list:
            c_info = {}
            c_info["type_"] = type_

            if not counter.binding:
                c_info["binding"] = ("global", None)
            elif counter.binding[0] == p4.P4_DIRECT:
                table = get_table_name(counter.binding[1])
                c_info["binding"] = ("direct", table)
                t_info = render_dict["table_info"][table]
                counter_key = type_ + "_counter"
                assert(not t_info[counter_key])
                t_info[counter_key] = name
            elif counter.binding[0] == p4.P4_STATIC:
                table = get_table_name(counter.binding[1])
                c_info["binding"] = ("static", table)
            else:
                assert(False)

            c_info["saturating"] = counter.saturating
            c_info["instance_count"] = counter.instance_count
            c_info["min_width"] = counter.min_width

            counter_info[name] = c_info

        # some kind of dirty hack, even though the one counter has been replaced
        # by one bytes counter and one packets counter, we still need this
        # information (mapping name -> type) for the implementation of COUNT()
        # in actions.c
        if str(counter.type) == "packets_and_bytes":
            c_info = {}
            c_info["type_"] = "packets_and_bytes"
            c_info["binding"] = ("global", None)
            c_info["saturating"] = counter.saturating
            c_info["instance_count"] = counter.instance_count
            c_info["min_width"] = counter.min_width

            counter_info[counter.name] = c_info

    render_dict["counter_info"] = counter_info

def render_dict_populate_meters(render_dict, hlir):
    meter_info = {}
    for name, meter in hlir.p4_meters.items():
        m_info = {}
        m_info["type_"] = str(meter.type)

        if not meter.binding:
            m_info["binding"] = ("global", None)
        elif meter.binding[0] == p4.P4_DIRECT:
            table = get_table_name(meter.binding[1])
            m_info["binding"] = ("direct", table)
            assert(meter.result)
            m_info["result"] = get_field_instance_name(meter.result)
            t_info = render_dict["table_info"][table]
            meter_key = "meter"
            assert(not t_info[meter_key])
            t_info[meter_key] = name
        elif meter.binding[0] == p4.P4_STATIC:
            table = get_table_name(meter.binding[1])
            m_info["binding"] = ("static", table)
        else:
            assert(False)

        m_info["instance_count"] = meter.instance_count

        meter_info[name] = m_info
    render_dict["meter_info"] = meter_info

def render_dict_populate_registers(render_dict, hlir):
    register_info = {}
    for name, register in hlir.p4_registers.items():
        r_info = {}
        if not register.width:
            print "Layout registers are not supported for now"
            assert(False)
        r_info["width"] = register.width
        r_info["byte_width"] = max(bytes_round_up(register.width), 4)
        r_info["mask"] = int_to_byte_array((1 << register.width) - 1, r_info["byte_width"])

        if not register.binding:
            r_info["binding"] = ("global", None)
        elif register.binding[0] == p4.P4_DIRECT:
            table = get_table_name(register.binding[1])
            r_info["binding"] = ("direct", table)
            t_info = render_dict["table_info"][table]
            t_info["registers"].append(name)
        elif register.binding[0] == p4.P4_STATIC:
            r_info["binding"] = ("static", table)
        else:
            assert(False)

        r_info["instance_count"] = register.instance_count

        register_info[name] = r_info
    render_dict["register_info"] = register_info

def get_type(byte_width):
    if byte_width == 1:
        return "uint8_t"
    elif byte_width == 2:
        return "uint16_t"
    elif byte_width <= 4:
        return "uint32_t"
    else:
        return "uint8_t *"

def get_thrift_type(byte_width):
    if byte_width == 1:
        return "byte"
    elif byte_width == 2:
        return "i16"
    elif byte_width <= 4:
        return "i32"
    elif byte_width == 6:
        return "MacAddr_t"
    elif byte_width == 16:
        return "IPv6_t"
    else:
        return "binary"

def render_dict_create(hlir,
                       p4_name, p4_prefix,
                       meta_config,
                       public_inc_path,
                       dump_yaml = False):
    render_dict = {}

    render_dict["p4_name"] = p4_name
    render_dict["public_inc_path"] = public_inc_path
    render_dict["p4_prefix"] = p4_prefix
    render_dict["p4_pd_prefix"] = "p4_pd_" + p4_prefix + "_"
    render_dict["get_type"] = get_type
    render_dict["get_thrift_type"] = get_thrift_type

    if not meta_config:
        meta_config_json = json.loads(resource_string(__name__, 'meta_config.json'))
    else:
        with open(meta_config, 'r') as fp:
            meta_config_json = json.load(fp)

    render_dict_populate_fields(render_dict, hlir, meta_config_json)
    render_dict_populate_data_types(render_dict, hlir)
    render_dict_populate_parse_states(render_dict, hlir)
    render_dict_populate_actions(render_dict, hlir)
    render_dict_populate_tables(render_dict, hlir)
    render_dict_populate_table_types(render_dict, hlir)
    render_dict_populate_action_profiles(render_dict, hlir)
    render_dict_populate_conditional_tables(render_dict, hlir)
    render_dict_populate_field_lists(render_dict, hlir)
    render_dict_populate_field_list_calculations(render_dict, hlir)
    render_dict_populate_counters(render_dict, hlir)
    render_dict_populate_meters(render_dict, hlir)
    render_dict_populate_registers(render_dict, hlir)

    if hlir.p4_egress_ptr:
        render_dict["egress_entry_table"] = get_table_name(hlir.p4_egress_ptr)
    else:
        render_dict["egress_entry_table"] = None

    if dump_yaml:
        with open("yaml_dump.yml", 'w') as f:
            dump_render_dict(render_dict, f)
    return render_dict

def ignore_template_file(filename):
    """
    Ignore these files in template dir
    """
    pattern = re.compile('^\..*|.*\.cache$|.*~$')
    return pattern.match(filename)

def gen_file_lists(current_dir, gen_dir, public_inc_path):
    """
    Generate target files from template; only call once
    """
    files_out = []
    if not public_inc_path:
        public_inc_path = os.path.join(gen_dir, "public_inc")
    for root, subdirs, files in os.walk(current_dir):
        for filename in files:
            if ignore_template_file(filename):
                continue
            relpath = os.path.relpath(os.path.join(root, filename), current_dir)
            template_file = relpath

            # Put the include files in public include path.
            if len(root) > 4 and root[-4:] == "/inc":
                target_file = os.path.join(public_inc_path, filename)
            else:
                target_file = os.path.join(gen_dir, relpath)
            files_out.append((template_file, target_file))
    return files_out

def render_all_files(render_dict, gen_dir, with_thrift = False, with_plugin_list=[]):
    files = gen_file_lists(templates_dir, gen_dir, render_dict["public_inc_path"])
    for template, target in files:
        # not very robust
        if (not with_thrift) and ("thrift" in target):
            continue
        path = os.path.dirname(target)
        if not os.path.exists(path):
            os.mkdir(path)
        with open(target, "w") as f:
            render_template(f, template, render_dict, templates_dir,
                            prefix = gl.tenjin_prefix)
    if len(with_plugin_list) > 0:
        for s in with_plugin_list:
            plugin_dir =  plugin_base + s
            plugin_files = gen_file_lists(plugin_dir, gen_dir+'/plugin/'+s, render_dict["public_inc_path"])
            for template, target in plugin_files:
                path = os.path.dirname(target)
                if not os.path.exists(path):
                    os.makedirs(path)
                with open(target, "w") as f:
                    render_template(f, template, render_dict, plugin_dir,
                            prefix = gl.tenjin_prefix)


