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

#ifndef _RMT_METADATA_UTILS_H
#define _RMT_METADATA_UTILS_H

void metadata_extract(uint8_t *dst, uint8_t *hdr,
		      int *metadata_recirc);

void metadata_dump(uint8_t *hdr, uint8_t *src);

//:: for name, binding in metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata);

void metadata_set_${name}(uint8_t *metadata, uint32_t value);

//::   #endfor


//:: if enable_intrinsic :
//::   for name, binding in intrinsic_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata);

void metadata_set_${name}(uint8_t *metadata, uint32_t value);

//::   #endfor
//:: #endif 

//:: if enable_pre :
//::   for name, binding in pre_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata);

void metadata_set_${name}(uint8_t *metadata, uint32_t value);

//::   #endfor
//:: #endif 

//:: for name, binding in extra_metadata_name_map.items():
uint32_t metadata_get_${name}(uint8_t *metadata);

void metadata_set_${name}(uint8_t *metadata, uint32_t value);

//:: #endfor
#endif
