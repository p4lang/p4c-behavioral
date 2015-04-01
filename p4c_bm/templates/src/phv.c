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

#include "phv.h"
#include "stdlib.h"

phv_data_t *phv_init(int phv_id, int pipeline_type) {
  phv_data_t *phv = malloc(sizeof(phv_data_t));
  phv->id = phv_id;
  phv->headers = malloc(PHV_LENGTH);
  phv->metadata_resubmit = metadata_recirc_create();
  phv->metadata_recirc = metadata_recirc_create();
  phv->pipeline_type = pipeline_type;
//:: num_virtual_fields = len(ordered_field_instances_virtual)
  phv->field_virtual_instances = calloc(${num_virtual_fields + 1},
					sizeof(rmt_field_instance_t));
//:: num_virtual_headers = len(ordered_header_instances_virtual)
  phv->header_virtual_instances = calloc(${num_virtual_headers + 1},
					 sizeof(rmt_header_instance_t));
  return phv;
}

/* Data for metadata initialization */
uint8_t metadata_initializer[] = {
//:: for header in ordered_header_instances_metadata:
//::   h_info = header_info[header]
//::   for field in h_info["fields"]:
//::     f_info = field_info[field]
  /* ${field} */
//::     bytes = [str(b) + ", " for b in f_info["default"]]
  ${''.join(bytes)}
//::   #endfor
//:: #endfor
};

//:: num_virtual_fields = len(ordered_field_instances_virtual)
//:: num_virtual_headers = len(ordered_header_instances_virtual)
static void reset_virtual_instances(phv_data_t *phv) {
  /* this takes care of the "last" instances, which need to be set to
     RMT_*_INSTANCE_NONE, i.e. 0 */
  memset(phv->header_virtual_instances, 0,
	 ${num_virtual_headers} * sizeof(rmt_header_instance_t));
  memset(phv->field_virtual_instances, 0,
	 ${num_virtual_fields} * sizeof(rmt_field_instance_t));
//:: for header_instance in ordered_header_instances_virtual:
//::     h_info = header_info[header_instance]
//::     if h_info["virtual_type"] == "next":
//::         first_tag = tag_stacks_first[h_info["base_name"]]
  phv->header_virtual_instances[RMT_HEADER_INSTANCE_${header_instance}] =\
    RMT_HEADER_INSTANCE_${first_tag};
//::         for i in xrange(len(h_info["fields"])):
//::             first_tag_field = header_info[first_tag]["fields"][i]
  phv->field_virtual_instances[RMT_FIELD_INSTANCE_${h_info["fields"][i]}] =\
    RMT_FIELD_INSTANCE_${first_tag_field};
//::         #endfor
//::     #endif

//:: #endfor
}

void phv_clean(phv_data_t *phv) {
  memset(phv->headers + PHV_HEADER_START, 0, PHV_HEADER_LENGTH);
  /* TODO: replace by metadata initializer */
  /* memset(phv->headers + PHV_METADATA_START, 0, PHV_METADATA_LENGTH); */
  memcpy(phv->headers + PHV_METADATA_START,
	 metadata_initializer, PHV_METADATA_LENGTH);
  memset(phv->headers_valid, 0, sizeof(phv->headers_valid));
  phv->resubmit_signal = 0;
  metadata_recirc_empty(phv->metadata_resubmit);
  phv->deparser_drop_signal = 0;
  phv->truncated_length = 0;
  metadata_recirc_empty(phv->metadata_recirc);
  reset_virtual_instances(phv);
//:: for header in meta_carry:
  metadata_recirc_add_header(phv->metadata_recirc,
			     RMT_HEADER_INSTANCE_${header});
//:: #endfor
}
