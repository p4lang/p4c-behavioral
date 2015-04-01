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

#include "lf.h"
#include "deparser.h"
#include "enums.h"
#include "rmt_internal.h"
#include "phv.h"

#include <p4_sim/pd.h>
#include <p4utils/tommylist.h>
#include <p4utils/tommyhashtbl.h>
#include <p4utils/circular_buffer.h>

#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#define LF_MAX_BUFFER_SIZE 128
#define LF_CB_SIZE 1024

//:: pd_prefix = ["p4_pd", p4_prefix]

typedef struct hash_object_s {
  tommy_node node;
  tommy_uint32_t value;
} hash_object_t;

int compare(const void *arg, const void *object) {
  // If there is a hash-collision, we will drop the lq. This is acceptable since
  // hardware does that too. If there are too many collisions in practice,
  // consider changing from 32-bit to 64-bit hash instead of having chains.
  assert(*(tommy_uint32_t *)arg == ((const hash_object_t *)object)->value);
  return *(tommy_uint32_t *)arg != ((const hash_object_t *)object)->value;
}

typedef struct lf_client_s lf_client_t;
typedef struct lf_busy_client_s lf_busy_client_t;

struct lf_client_s {
  tommy_node node;
  p4_pd_sess_hdl_t sess_hdl;
  void *client_data;
  union {
//:: for lq in learn_quanta:
    ${lq["cb_fn_type"]} ${lq["name"]}_cb_fn;
//:: #endfor
  } cb_fn;
  bool is_invoking_cb_fn;
};

struct lf_busy_client_s {
  tommy_node node;
  p4_pd_sess_hdl_t sess_hdl;
  void *digest_msg;
};

typedef enum lf_buffer_state_e {IDLE, FILL, DRAIN} lf_buffer_state_t;
typedef struct lf_buffer_s {
//:: for lq in learn_quanta:
  ${lq["msg_type"]} ${lq["name"]}_buffer;
//:: #endfor
  tommy_list busy_clients;
  tommy_hashtable hashtable;
  lf_buffer_state_t state;
} lf_buffer_t;

static void
lf_buffer_reset(lf_buffer_t *lf_buffer) {
  RMT_LOG(P4_LOG_LEVEL_TRACE, "Resetting buffer\n");
  lf_buffer->state = IDLE;
//:: for lq in learn_quanta:
  {
    int i;
    for (i = 0; i < lf_buffer->${lq["name"]}_buffer.num_entries; ++i) {
      tommy_uint32_t value = tommy_hash_u32(0, &lf_buffer->${lq["name"]}_buffer.entries[i], sizeof(${lq["entry_type"]}));
      free(tommy_hashtable_remove(&lf_buffer->hashtable, compare, &value, value));
    }
    lf_buffer->${lq["name"]}_buffer.num_entries = 0;
  }
//:: #endfor
}

//:: if len(learn_quanta) > 0:
static lf_client_t*
find_client(tommy_list *clients, const p4_pd_sess_hdl_t sess_hdl) {
  tommy_node *node = tommy_list_head(clients);
  while (NULL != node) {
    lf_client_t *client = (lf_client_t *)node->data;
    if (client->sess_hdl == sess_hdl) return client;
    node = node->next;
  }

  return NULL;
}

static lf_busy_client_t*
find_busy_client(tommy_list *busy_clients, const p4_pd_sess_hdl_t sess_hdl, void *digest_msg) {
  tommy_node *node = tommy_list_head(busy_clients);
  while (NULL != node) {
    lf_busy_client_t *busy_client = (lf_busy_client_t *)node->data;
    if (busy_client->sess_hdl == sess_hdl && busy_client->digest_msg == digest_msg) return busy_client;
    node = node->next;
  }

  return NULL;
}

/*
 * This function removes a client from a buffer's |busy_clients| list. If the
 * list becomes empty as a result of this operation, the state of the buffer is
 * set to |IDLE|.
 */
static void
remove_busy_client(lf_buffer_t *buffer, lf_busy_client_t *busy_client) {
  assert(buffer->state == DRAIN);
  void *data = tommy_list_remove_existing(&buffer->busy_clients, &busy_client->node);
  free(data);
  if ((NULL != data) && tommy_list_empty(&buffer->busy_clients)) {
    lf_buffer_reset(buffer);
  }
}
//:: #endif

typedef struct lf_digest_entry_s {
  rmt_field_list_t type;
  union {
//:: for lq in learn_quanta:
    ${lq["entry_type"]} ${lq["name"]}_entry;
//:: #endfor
  } entry;
} lf_digest_entry_t;

/*
 * The pointers below refer to those two buffers only one of which is active at
 * a time.
 */
static lf_buffer_t* lf_current_buffer = NULL;
static lf_buffer_t* lf_other_buffer = NULL;
// Always acquire this before accessing the buffer pointers.
static pthread_mutex_t lf_lock;

#define DEFAULT_WAIT_TIME 2
static struct timeval lf_wait_time = { .tv_sec = DEFAULT_WAIT_TIME, .tv_usec = 0 };

static lf_buffer_t*
lf_buffer_create() {
  lf_buffer_t *lf_buffer = (lf_buffer_t *)malloc(sizeof(lf_buffer_t));

//:: for lq in learn_quanta:
  lf_buffer->${lq["name"]}_buffer.num_entries = 0;
  lf_buffer->${lq["name"]}_buffer.entries = malloc(sizeof(${lq["entry_type"]}) * LF_MAX_BUFFER_SIZE);
//:: #endfor

  // We do not expect more than LF_MAX_BUFFER_SIZE entries in the hash table.
  // But, according to documentation, performance of the static hash table
  // begins to deteriorate when load factor reaches 0.75. Multipying by 2 will
  // keep load factor below 0.5.
  tommy_hashtable_init(&lf_buffer->hashtable, LF_MAX_BUFFER_SIZE * 2);

  lf_buffer->state = IDLE;

  return lf_buffer;
}

static void
lf_buffer_destroy(lf_buffer_t *buffer) {
  if (NULL != buffer) {
    lf_buffer_reset(buffer);
//:: for lq in learn_quanta:
    free(buffer->${lq["name"]}_buffer.entries);
//:: #endfor
    // The hashtable should have been emptied by the buffer reset.
    assert(0 == tommy_hashtable_count(&buffer->hashtable));
    tommy_hashtable_done(&buffer->hashtable);
    tommy_list_foreach(&(buffer->busy_clients), free);
    free(buffer);
  }
}

static void
lf_buffer_swap(lf_buffer_t **buffer1, lf_buffer_t **buffer2) {
  RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Swapping buffers\n");
  assert(NULL != buffer1);
  assert(NULL != buffer2);
  lf_buffer_t *tmp = *buffer1;
  *buffer1 = *buffer2;
  *buffer2 = tmp;
}

static void
lf_buffer_reset_and_swap(lf_buffer_t **current_buffer, lf_buffer_t **other_buffer) {
  assert((*other_buffer)->state != FILL);
  assert((*current_buffer)->state != IDLE);

  if ((*other_buffer)->state == IDLE && (*current_buffer)->state == DRAIN) {
    // Swap current and other buffer.
    lf_buffer_swap(current_buffer, other_buffer);
    (*current_buffer)->state = FILL;
  }
}

//:: lf_prefix = ["lf", p4_prefix]
//:: lf_set_learning_timeout = "_".join(lf_prefix + ["set_learning_timeout"])
void
${lf_set_learning_timeout}(const long int usecs) {
  // Need to acquire lock to modify |lf_wait_time|.
  pthread_mutex_lock(&lf_lock);
  lf_wait_time.tv_sec = (time_t)(usecs / 1000000);
  lf_wait_time.tv_usec = usecs % 1000000;
  pthread_mutex_unlock(&lf_lock);
}

//:: for lq in learn_quanta:
// Linked list to hold registered clients.
static tommy_list ${lq["name"]}_clients;

p4_pd_status_t
${lq["lf_register_fn"]}(p4_pd_sess_hdl_t sess_hdl, ${lq["cb_fn_type"]} cb_fn, void *client_data) {
  if (NULL != find_client(&${lq["name"]}_clients, sess_hdl)) {
    pthread_mutex_unlock(&lf_lock);

    RMT_LOG(P4_LOG_LEVEL_TRACE, "Received duplicate registration for %u\n", sess_hdl);
    return (p4_pd_status_t)1;
  }
  else {
    RMT_LOG(P4_LOG_LEVEL_TRACE, "Received registration for %u\n", sess_hdl);
    lf_client_t *client = malloc(sizeof(lf_client_t));
    client->sess_hdl = sess_hdl;
    client->cb_fn.${lq["name"]}_cb_fn = cb_fn;
    client->client_data = client_data;
    client->is_invoking_cb_fn = false;
    tommy_list_insert_tail(&${lq["name"]}_clients, &client->node, client);

    pthread_mutex_unlock(&lf_lock);

    return (p4_pd_status_t)0;
  }
}

p4_pd_status_t
${lq["lf_deregister_fn"]}(p4_pd_sess_hdl_t sess_hdl) {
  pthread_mutex_lock(&lf_lock);

  lf_client_t * const client = find_client(&${lq["name"]}_clients, sess_hdl);
  if (NULL == client) {
    pthread_mutex_unlock(&lf_lock);
    return (p4_pd_status_t)1;
  }
  else if (true == client->is_invoking_cb_fn) {
    pthread_mutex_unlock(&lf_lock);

    RMT_LOG(P4_LOG_LEVEL_WARN, "Deregister failed, client %d in use\n", sess_hdl);

    return (p4_pd_status_t)1;
  }
  else {
    tommy_list_remove_existing(&${lq["name"]}_clients, &client->node);
    free(client);

    pthread_mutex_unlock(&lf_lock);

    return (p4_pd_status_t)0;
  }
}

p4_pd_status_t
${lq["lf_notify_ack_fn"]}(p4_pd_sess_hdl_t sess_hdl, ${lq["msg_type"]} *digest_msg) {
  pthread_mutex_lock(&lf_lock);
  if (lf_other_buffer->state == DRAIN) {
    lf_busy_client_t *busy_client = find_busy_client(&lf_other_buffer->busy_clients, sess_hdl, digest_msg);
    if (busy_client) {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Found client %u in other buffer\n", sess_hdl);
      // This function may modify the |lf_other_buffer->state|.
      remove_busy_client(lf_other_buffer, busy_client);
      lf_buffer_reset_and_swap(&lf_current_buffer, &lf_other_buffer);
    }
    else {
      if (lf_current_buffer->state == DRAIN) {
        busy_client = find_busy_client(&lf_current_buffer->busy_clients, sess_hdl, digest_msg);
        if (busy_client) {
          RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Found client %u in current buffer\n", sess_hdl);
          remove_busy_client(lf_current_buffer, busy_client);
          if (IDLE == lf_current_buffer->state) {
            lf_current_buffer->state = FILL;
          }
        }
      }
      else {
        RMT_LOG(P4_LOG_LEVEL_WARN, "Cannot find client for session %u, digest %p\n", sess_hdl, digest_msg);
      }
    }
  }
  else {
    // |lf_other_buffer| cannot be in FILL state.
    assert(lf_other_buffer->state == IDLE);
    // If |lf_other_buffer| is in IDLE state, |lf_current_buffer| should be in
    // fill state.
    assert(lf_current_buffer->state == FILL);
    RMT_LOG(P4_LOG_LEVEL_WARN, "Received notify_ack for %d when no buffer in drain state\n", sess_hdl);
  }
  pthread_mutex_unlock(&lf_lock);

  return 0;
}
//:: #endfor

static circular_buffer_t *lf_cb = NULL; /* only 1 */

static void *lf_loop(void *arg) {
  struct timeval first_digest_time;
  int n = 0;
  while (1) {

    pthread_mutex_lock(&lf_lock);

    struct timeval wait_time = lf_wait_time;
    if (n > 0) {
      struct timeval flush_time, now;
      gettimeofday(&now, NULL);
      timeradd(&first_digest_time, &lf_wait_time, &flush_time);
      timersub(&flush_time, &now, &wait_time);
    }

    pthread_mutex_unlock(&lf_lock);

    lf_digest_entry_t *digest_entry = (lf_digest_entry_t *)cb_read_with_wait(lf_cb, &wait_time);

    pthread_mutex_lock(&lf_lock);

    assert(lf_current_buffer->state != IDLE);
    assert(lf_other_buffer->state != FILL);
    // If |lf_current_buffer->state| is |DRAIN|, then |lf_other_buffer->state| should also be |DRAIN|
    assert((lf_current_buffer->state == FILL) || (lf_other_buffer->state == DRAIN));

    if (NULL != digest_entry) {
      if (lf_current_buffer->state == FILL) {
        switch (digest_entry->type) {
//:: for lq in learn_quanta:
          case RMT_FIELD_LIST_${lq["name"]}: {
            tommy_uint32_t value = tommy_hash_u32(0, &digest_entry->entry.${lq["name"]}_entry, sizeof(${lq["entry_type"]}));
            hash_object_t *object = (hash_object_t *)tommy_hashtable_search(&lf_current_buffer->hashtable, compare, &value, value);
            if ((NULL == object) &&
                !(lf_other_buffer->state == DRAIN && NULL != tommy_hashtable_search(&lf_other_buffer->hashtable, compare, &value, value))) {
              RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Learning RMT_FIELD_LIST_${lq["name"]}\n");

              // Insert newly learnt value into hashtable and field-list-specific buffer.
              object = (hash_object_t *)malloc(sizeof(hash_object_t));
              object->value = value;
              tommy_hashtable_insert(&lf_current_buffer->hashtable, &object->node, object, value);
              memcpy(&lf_current_buffer->${lq["name"]}_buffer.entries[lf_current_buffer->${lq["name"]}_buffer.num_entries],
                &digest_entry->entry.${lq["name"]}_entry, sizeof(${lq["entry_type"]}));
              ++lf_current_buffer->${lq["name"]}_buffer.num_entries;

              if (n == 0) {
                gettimeofday(&first_digest_time, NULL);
              }

              ++n;
            }
            else {
              RMT_LOG(P4_LOG_LEVEL_TRACE, "Hash collision for RMT_FIELD_LIST_${lq["name"]}\n");
            }
            break;
          }
//:: #endfor
          default: assert(0);
        }
      }
      else {
        RMT_LOG(P4_LOG_LEVEL_INFO, "Dropping digest %d\n", digest_entry->type);
      }
    }

    free(digest_entry);

    lf_buffer_t * const drain_buffer = lf_current_buffer;
    (void)drain_buffer;

//:: for lq in learn_quanta:
    tommy_list ${lq["name"]}_queued_clients;
    tommy_list_init(&${lq["name"]}_queued_clients);
//:: #endfor

    if ((lf_current_buffer->state == FILL) && ((n >= LF_MAX_BUFFER_SIZE) ||
        ((n > 0) && (NULL == digest_entry)))) {
      RMT_LOG(P4_LOG_LEVEL_VERBOSE, "Reporting %d digests\n", n);

      // Change the state of the buffer to |IDLE|. It will remain in this state
      // if there are no registered clients.
      lf_current_buffer->state = DRAIN;

//:: for lq in learn_quanta:
      {
        if (drain_buffer->${lq["name"]}_buffer.num_entries > 0) {
          RMT_LOG(P4_LOG_LEVEL_TRACE, "Reporting %hu ${lq["name"]} digests\n", drain_buffer->${lq["name"]}_buffer.num_entries);
          tommy_node *node = tommy_list_head(&${lq["name"]}_clients);
          while (NULL != node) {
            lf_client_t *client = (lf_client_t *)node->data;
         
            lf_busy_client_t *busy_client = (lf_busy_client_t *)malloc(sizeof(lf_busy_client_t));
            memcpy(&busy_client->sess_hdl, &client->sess_hdl, sizeof(p4_pd_sess_hdl_t));
            busy_client->digest_msg = (void *)&drain_buffer->${lq["name"]}_buffer;
            tommy_list_insert_tail(&drain_buffer->busy_clients, &busy_client->node, busy_client);

            lf_client_t *queued_client = (lf_client_t *)malloc(sizeof(lf_client_t));
            queued_client->sess_hdl = client->sess_hdl;
            queued_client->client_data = client->client_data;
            queued_client->cb_fn.${lq["name"]}_cb_fn = client->cb_fn.${lq["name"]}_cb_fn;
            client->is_invoking_cb_fn = true;
            tommy_list_insert_tail(&${lq["name"]}_queued_clients, &(queued_client->node), queued_client);
            node = node->next;
          }
        }
      }
//:: #endfor
      n = 0;

      lf_buffer_reset_and_swap(&lf_current_buffer, &lf_other_buffer);

      assert(true
//:: for lq in learn_quanta:
        && (!tommy_list_empty(&${lq["name"]}_queued_clients) ||
            drain_buffer->${lq["name"]}_buffer.num_entries == 0 ||
            tommy_list_empty(&${lq["name"]}_clients))
//:: #endfor
      );

      if (true
//:: for lq in learn_quanta:
        && tommy_list_empty(&${lq["name"]}_queued_clients)
//:: #endfor
      ) {
        // This leg of code is executed when there are no registered clients.
        assert(DRAIN == drain_buffer->state);
        assert(FILL == lf_current_buffer->state);
        assert(drain_buffer == lf_other_buffer);
        lf_buffer_reset(drain_buffer);
      }
    }

    pthread_mutex_unlock(&lf_lock);

    tommy_node *queued_node = NULL;
    (void)queued_node;
//:: for lq in learn_quanta:
    queued_node = tommy_list_head(&${lq["name"]}_queued_clients);
    while (NULL != queued_node) {
      assert(drain_buffer->state == DRAIN);
      lf_client_t *client = (lf_client_t *)queued_node->data;
      client->is_invoking_cb_fn = true;
      client->cb_fn.${lq["name"]}_cb_fn(client->sess_hdl, &drain_buffer->${lq["name"]}_buffer, client->client_data);
      queued_node = queued_node->next;
    }
    tommy_list_foreach(&${lq["name"]}_queued_clients, free);
//:: #endfor

    pthread_mutex_lock(&lf_lock);

//:: for lq in learn_quanta:
    {
      tommy_node *node = tommy_list_head(&${lq["name"]}_clients);
      while (NULL != node) {
        ((lf_client_t *)(node->data))->is_invoking_cb_fn = false;
        node = node->next;
      }
    }
//:: #endfor

    pthread_mutex_unlock(&lf_lock);
  }
}
 
int lf_receive(phv_data_t *phv, rmt_field_list_t field_list) {
  switch (field_list) {
    case RMT_FIELD_LIST_NONE: break;
//:: for lq in learn_quanta:
    case RMT_FIELD_LIST_${lq["name"]}: {
      RMT_LOG(P4_LOG_LEVEL_TRACE, "Received digest for RMT_FIELD_LIST_${lq["name"]}\n");
      lf_digest_entry_t *digest_entry = (lf_digest_entry_t *)malloc(sizeof(lf_digest_entry_t));
      digest_entry->type = field_list;
      ${lq["entry_type"]} *entry = &(digest_entry->entry.${lq["name"]}_entry);
      uint8_t *src = NULL;
//::   for field in lq["fields"].keys():
//::     f_info = field_info[field]
//::     d_type = f_info["data_type"]
//::     byte_width_phv = f_info["byte_width_phv"]
//::     bit_offset_hdr = f_info["bit_offset_hdr"]
//::     bit_width = f_info["bit_width"]
      src = get_phv_ptr(phv, ${f_info["byte_offset_phv"]});
//::     if d_type == "uint32_t":
      {
        uint32_t dst = *(uint32_t*)src;
        // Send to host in host-byte order.
        entry->${field} = ntohl(dst);
      }
//::     elif d_type == "byte_buf_t":
      deparse_byte_buf(entry->${field}, src, ${byte_width_phv});
//::     #endif
//::   #endfor
      return cb_write(lf_cb, digest_entry);
    }
//:: #endfor
    default: assert(0);
  }
  return 0;
}

static pthread_t lf_thread;

static void
lf_init_buffers() {
  lf_buffer_destroy(lf_current_buffer);
  lf_buffer_destroy(lf_other_buffer);

  lf_current_buffer = lf_buffer_create();
  lf_other_buffer = lf_buffer_create();
  lf_current_buffer->state = FILL;
  tommy_list_init(&lf_current_buffer->busy_clients);
  tommy_list_init(&lf_other_buffer->busy_clients);
}

void lf_init() {
  lf_cb = cb_init(LF_CB_SIZE, CB_WRITE_BLOCK, CB_READ_BLOCK);

  lf_init_buffers();

//:: for lq in learn_quanta:
  tommy_list_init(&${lq["name"]}_clients);
//:: #endfor

  pthread_mutex_init(&lf_lock, NULL);

  pthread_create(&lf_thread, NULL, lf_loop, NULL);
}

void lf_clean_all() {
  pthread_mutex_lock(&lf_lock);

  lf_init_buffers();
//:: for lq in learn_quanta:
  tommy_list_foreach(&${lq["name"]}_clients, free);
  tommy_list_init(&${lq["name"]}_clients);
//:: #endfor

  lf_wait_time.tv_sec = DEFAULT_WAIT_TIME;
  lf_wait_time.tv_usec = 0;

  pthread_mutex_unlock(&lf_lock);
}
