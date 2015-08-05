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
*/#ifdef P4RMT_OUTPUT_STATS

#include <whitedb/dbapi.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <Judy.h>

#include "stats.h"

pthread_mutex_t stats_lock;
pthread_mutex_t events_count_lock;

void *stats_db;
Pvoid_t events_count;

#define STATS_PACKET_IN_PARSE_STATE 1
#define STATS_PACKET_APPLY_TABLE 2

#define MAX_SIZE 20000000 /* 20 megabytes */
#define DB_NAME "1000" /* has to be an integer */

int stats_init() {
  assert(!stats_db);
  stats_db = wg_attach_database(DB_NAME, MAX_SIZE);
  events_count = (Pvoid_t) NULL;
  pthread_mutex_init(&stats_lock, NULL);
  pthread_mutex_init(&events_count_lock, NULL);
  return (stats_db == NULL);
}

int stats_clean_up() {
  return wg_detach_database(stats_db);
}

/* int stats_dump_to_disk(const char *filename) { */
/*   pthread_mutex_lock(&stats_lock); */
/*   wg_int res = wg_dump(stats_db, (char *) filename); */
/*   pthread_mutex_unlock(&stats_lock); */
/*   return (res == WG_ILLEGAL); */
/* } */

/* int stats_dump_and_delete(const char *filename) { */
/*   int res = stats_dump_to_disk(filename); */
/*   res |= wg_detach_database(stats_db); */
/*   res |= wg_delete_database(stats_db); */
/*   pthread_mutex_destroy(&stats_lock); */
/*   return res; */
/* } */

Word_t * PValue;
int      Rc_int;

int stats_packet_in(uint64_t packet_id) {
  pthread_mutex_lock(&events_count_lock);
  JLI(PValue, events_count, packet_id);
  if(PValue == PJERR) {
    pthread_mutex_unlock(&events_count_lock);
    return -1;
  }
  *PValue = 0;
  pthread_mutex_unlock(&events_count_lock);
  return 0;
}

int stats_packet_out(uint64_t packet_id) {
  pthread_mutex_lock(&events_count_lock);
  JLD(Rc_int, events_count, packet_id);
  if(Rc_int == JERR) {
    pthread_mutex_unlock(&events_count_lock);
    return -1;
  }
  pthread_mutex_unlock(&events_count_lock);
  return 0;
}

static inline int get_packet_event_number(uint64_t packet_id) {
  pthread_mutex_lock(&events_count_lock);
  JLF(PValue, events_count, packet_id);
  if(PValue == NULL) {
    pthread_mutex_unlock(&events_count_lock);
    return -1;
  }
  int res = *PValue;
  *PValue = ++res;
  pthread_mutex_unlock(&events_count_lock);
  return res;
}

int stats_packet_visit_table(uint64_t packet_id,
			     int pipeline_type, int pipeline_id,
			     char *table, char *action) {
  int event_num = get_packet_event_number(packet_id);
  if(event_num == -1) return -1;
  pthread_mutex_lock(&stats_lock);
  void *rec = wg_create_record(stats_db, 7);
  if(!rec){
    pthread_mutex_unlock(&stats_lock);
    return -1;
  }
  int res = wg_set_int_field(stats_db, rec, 0, STATS_PACKET_APPLY_TABLE);
  res |= wg_set_int_field(stats_db, rec, 1, packet_id);
  res |= wg_set_int_field(stats_db, rec, 2, pipeline_type);
  res |= wg_set_int_field(stats_db, rec, 3, pipeline_id);
  res |= wg_set_int_field(stats_db, rec, 4, event_num);
  res |= wg_set_str_field(stats_db, rec, 5, table);
  res |= wg_set_str_field(stats_db, rec, 6, action);
  pthread_mutex_unlock(&stats_lock);
  return res;
}

int stats_packet_visit_parse_state(uint64_t packet_id,
				   int pipeline_type, int pipeline_id,
				   char *parse_state) {
  int event_num = get_packet_event_number(packet_id);
  if(event_num == -1) return -1;
  pthread_mutex_lock(&stats_lock);
  void *rec = wg_create_record(stats_db, 6);
  if(!rec){
    pthread_mutex_unlock(&stats_lock);
    return -1;
  }
  int res = wg_set_int_field(stats_db, rec, 0, STATS_PACKET_IN_PARSE_STATE);
  res |= wg_set_int_field(stats_db, rec, 1, packet_id);
  res |= wg_set_int_field(stats_db, rec, 2, pipeline_type);
  res |= wg_set_int_field(stats_db, rec, 3, pipeline_id);
  res |= wg_set_int_field(stats_db, rec, 4, event_num);
  res |= wg_set_str_field(stats_db, rec, 5, parse_state);
  pthread_mutex_unlock(&stats_lock);
  return res;
}

#endif
