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

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

#include "stateful.h"
#include "rmt_internal.h"
#include "primitives.h"

static struct timeval time_init;

struct counter_s {
  uint64_t *instances;
  size_t num_instances;
  char *name;
  pthread_mutex_t lock;
};

struct reg_s {
  char *instances;
  size_t num_instances;
  size_t byte_width;
  pthread_mutex_t lock;
};

typedef struct meter_queue_s {
  int valid;
  uint64_t last_timestamp;  /* us */
  uint32_t tokens;
  double info_rate; /* packets per us */
  uint32_t burst_size;
  meter_color_t color;
} meter_queue_t;

typedef struct meter_instance_s {
  meter_queue_t queues[METER_EXCEED_ACTION_COLOR_END_];
  uint64_t counters[METER_EXCEED_ACTION_COLOR_END_];
} meter_instance_t;

struct meter_s {
  meter_instance_t *instances;
  size_t num_instances;
  char *name;
  pthread_mutex_t lock;
};

//:: for c_name, c_info in counter_info.items():
//::   type_ = c_info["type_"]
//::   if type_ == "packets_and_bytes": continue
//::   instance_count = c_info["instance_count"]
counter_t counter_${c_name};
//:: #endfor

//:: for m_name, m_info in meter_info.items():
meter_t meter_${m_name};
//:: #endfor

//:: for r_name, r_info in register_info.items():
reg_t reg_${r_name};
//:: #endfor

void stateful_init_counters(counter_t *counter, size_t num_instances, char *name) {
  counter->instances = calloc(num_instances, sizeof(uint64_t));
  counter->num_instances = num_instances;
  counter->name = name;
  pthread_mutex_init(&counter->lock, NULL);
}

void stateful_init_meters(meter_t *meter, size_t num_instances, char *name) {
  meter->instances = calloc(num_instances, sizeof(meter_instance_t));
  meter->num_instances = num_instances;
  meter->name = name;
  pthread_mutex_init(&meter->lock, NULL);
}

void stateful_init_registers(reg_t *reg,
			     size_t num_instances, size_t byte_width ) {
  reg->instances = calloc(num_instances, byte_width);
  reg->num_instances = num_instances;
  reg->byte_width = byte_width;
  pthread_mutex_init(&reg->lock, NULL);
}

uint64_t stateful_read_counter(counter_t *counter, int index) {
  uint64_t value;
  pthread_mutex_lock(&counter->lock);
  value = counter->instances[index];
  pthread_mutex_unlock(&counter->lock);
  return value;
}

void stateful_write_counter(counter_t *counter, int index, uint64_t value) {
  pthread_mutex_lock(&counter->lock);
  counter->instances[index] = value;
  pthread_mutex_unlock(&counter->lock);
}

void stateful_increase_counter(counter_t *counter, int index, uint64_t value) {
  pthread_mutex_lock(&counter->lock);
  counter->instances[index] += value;
  pthread_mutex_unlock(&counter->lock);
}

void stateful_reset_counter(counter_t *counter, int index) {
  pthread_mutex_lock(&counter->lock);
  counter->instances[index] = 0;
  pthread_mutex_unlock(&counter->lock);
}

void stateful_reset_meter(meter_t *meter, int index) {
  pthread_mutex_lock(&meter->lock);
  meter_instance_t *instance = &meter->instances[index];
  memset(instance, 0, sizeof(meter_instance_t));
  pthread_mutex_unlock(&meter->lock);
}

int stateful_meter_set_queue(meter_t *meter, int index,
			     double info_rate, uint32_t burst_size,
			     meter_color_t color) {
  pthread_mutex_lock(&meter->lock);
  meter_instance_t *instance = &meter->instances[index];
  meter_queue_t *new_queue = &instance->queues[color];
  new_queue->color = color;
  new_queue->info_rate = info_rate;
  new_queue->burst_size = burst_size;
  new_queue->tokens = burst_size;
  new_queue->last_timestamp = 0;
  new_queue->valid = 1;
  pthread_mutex_unlock(&meter->lock);
  return 0;
}

char *color_to_str(meter_color_t color) {
  switch(color) {
  case METER_EXCEED_ACTION_COLOR_GREEN: return "GREEN";
  case METER_EXCEED_ACTION_COLOR_YELLOW: return "YELLOW";
  case METER_EXCEED_ACTION_COLOR_RED: return "RED";
  default: assert(0);
  }
}

meter_color_t stateful_execute_meter(meter_t *meter, int index, uint32_t input) {
  pthread_mutex_lock(&meter->lock);

  meter_instance_t *instance = &meter->instances[index];

  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t time_since_init = (now.tv_sec - time_init.tv_sec) * 1000000 +
    (now.tv_usec - time_init.tv_usec);

  uint64_t usec_diff;

  uint32_t new_tokens;
  meter_queue_t *queue;
  

  meter_color_t color = METER_EXCEED_ACTION_COLOR_GREEN;

  int i;
  for(i = 1; i < METER_EXCEED_ACTION_COLOR_END_; i++) {
    queue = &instance->queues[i];
    if(!queue->valid) continue;
    usec_diff = time_since_init - queue->last_timestamp;
    new_tokens = (uint32_t) (usec_diff * queue->info_rate);
    
    if(new_tokens > 0) {
      queue->tokens += new_tokens;
      if(queue->tokens > queue->burst_size) {
	queue->tokens = queue->burst_size;
      }
      /* TODO : improve this ? */
      queue->last_timestamp += (uint32_t) (new_tokens / queue->info_rate);
    }

    RMT_LOG(P4_LOG_LEVEL_TRACE,
	    "adding %u tokens, now have %u\n",
	    new_tokens,
	    queue->tokens);

    if(queue->tokens < input) {
      color = queue->color;
    }
    else {
      queue->tokens -= input;
      break;
    }
  }

  instance->counters[color]++;

  pthread_mutex_unlock(&meter->lock);

  RMT_LOG(P4_LOG_LEVEL_VERBOSE,
	  "meter %s is marking packet %s\n",
	  meter->name,
	  color_to_str(color));

  return color;
}

//:: for r_name, r_info in register_info.items():
//::   byte_width = r_info["byte_width"]
void stateful_write_register_${r_name} (int index, uint8_t *src, int src_len) {
  char mask[] = {
//::   for byte in r_info["mask"]:
    ${byte},
//::   #endfor
  };
  char *dst = reg_${r_name}.instances + index * ${byte_width};
  pthread_mutex_lock(&reg_${r_name}.lock);
  COPY_GENERIC(dst, ${byte_width},
	       (char *) src, src_len,
	       mask, ${byte_width});
  pthread_mutex_unlock(&reg_${r_name}.lock);
}

//:: #endfor

//:: for r_name, r_info in register_info.items():
//::   byte_width = r_info["byte_width"]
void stateful_read_register_${r_name} (phv_data_t *phv, int index,
				       rmt_field_instance_t dst, int dst_len,
				       uint8_t *mask_ptr, int mask_len) {
  char *src = reg_${r_name}.instances + index * ${byte_width};
  pthread_mutex_lock(&reg_${r_name}.lock);
  MODIFY_FIELD_GENERIC(phv,
		       dst, dst_len,
		       (uint8_t *) src, ${byte_width},
		       mask_ptr, mask_len);
  pthread_mutex_unlock(&reg_${r_name}.lock);
}

//:: #endfor

void stateful_init(void) {
  gettimeofday(&time_init, NULL);
//:: for c_name, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct": continue
//::   if type_ == "packets_and_bytes": continue
//::   instance_count = c_info["instance_count"]
  stateful_init_counters(&counter_${c_name}, ${instance_count}, "${c_name}");
//:: #endfor

//:: for m_name, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   if binding[0] == "direct": continue
//::   instance_count = m_info["instance_count"]
  stateful_init_meters(&meter_${m_name}, ${instance_count}, "${m_name}");
//:: #endfor

//:: for r_name, r_info in register_info.items():
//::   binding = r_info["binding"]
//::   if binding[0] == "direct": continue
//::   instance_count = r_info["instance_count"]
//::   byte_width = r_info["byte_width"]
  stateful_init_registers(&reg_${r_name}, ${instance_count}, ${byte_width});
//:: #endfor
}

static void stateful_reset_counters(counter_t *counter) {
  pthread_mutex_lock(&counter->lock);
  memset(counter->instances, 0, counter->num_instances * sizeof(uint64_t));
  pthread_mutex_unlock(&counter->lock);
}

static void stateful_reset_meters(meter_t *meter) {
  pthread_mutex_lock(&meter->lock);
  memset(meter->instances, 0, meter->num_instances * sizeof(meter_instance_t));
  pthread_mutex_unlock(&meter->lock);
}

static void stateful_reset_registers(reg_t *reg) {
  pthread_mutex_lock(&reg->lock);
  memset(reg->instances, 0, reg->num_instances * reg->byte_width);
  pthread_mutex_unlock(&reg->lock);
}

void stateful_clean_all(void) {
//:: for c_name, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct": continue
//::   if type_ == "packets_and_bytes": continue
  stateful_reset_counters(&counter_${c_name});
//:: #endfor

//:: for m_name, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   if binding[0] == "direct": continue
  stateful_reset_meters(&meter_${m_name});
//:: #endfor

//:: for r_name, r_info in register_info.items():
//::   binding = r_info["binding"]
//::   if binding[0] == "direct": continue
  stateful_reset_registers(&reg_${r_name});
//:: #endfor
}
