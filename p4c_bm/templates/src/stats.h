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

#ifdef P4RMT_OUTPUT_STATS

#ifndef _RMT_STATS_H
#define _RMT_STATS_H

/* Only call it once. Returns 0 if success */
int stats_init(void);

int stats_clean_up(void);

/* Needs to be called when a new pakcet comes in. Returns 0 if success. */
int stats_packet_in(uint64_t packet_id);
int stats_packet_out(uint64_t packet_id);

/* /\* Returns 0 if success *\/ */
/* int stats_dump_to_disk(const char *filename); */
/* /\* Call at the very end. Returns 0 if success *\/ */
/* int stats_dump_and_delete(const char *filename); */

/* Returns 0 if success */
int stats_packet_visit_table(uint64_t packet_id,
			     int pipeline_type, int pipeline_id,
			     char *table, char *action);
/* Returnsn 0 if success */
int stats_packet_visit_parse_state(uint64_t packet_id,
				   int pipeline_type, int pipeline_id,
				   char *parse_state);

#endif

#endif
