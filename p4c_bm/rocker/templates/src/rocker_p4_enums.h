//:: # Template file
//:: # @FIXME top matter needed
//:: rocker_p4_prefix = "rocker_" + p4_prefix + "_"
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
/**
 *
 * DO NOT EDIT:  This file is generated from P4 program.
 *
 */

#ifndef _ROCKER_${p4_prefix}ENUMS_H
#define _ROCKER_${p4_prefix}ENUMS_H

/**
 * @addtogroup Tables
 * @{
 */

/**
 * @brief Enumeration of table names
 */
typedef enum ${rocker_p4_prefix}_table_e {
    RMT_TABLE_NONE,
//:: for table_name, t_info in table_info.items():
    RMT_TABLE_${table_name},
//:: #endfor
    RMT_TABLE_COUNT
} ${rocker_p4_prefix}_table_t;

#endif
