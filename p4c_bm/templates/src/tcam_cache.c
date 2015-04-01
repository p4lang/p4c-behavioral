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

#include "tcam_cache.h"

#include <p4utils/lookup3.h>
#include <p4utils/tommyhashtbl.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

typedef struct cache_entry_s{
  time_t last_access;
  uint8_t *key;
  int key_size; /* all the keys have to be the same size, this just make the
		   code slightly simpler */
  void *data;
  tommy_hashtable_node node;
} cache_entry_t;

struct tcam_cache_s {
  pthread_mutex_t lock;
  int size;
  int key_size;
  int expiration_secs;
  int nb_entries;
  cache_entry_t *entries;
  unsigned long *bitmap_used;
  int bitmap_size;
  tommy_hashtable hashtable;
};

static inline int cmp (const void *arg, const void *obj) {
  return memcmp(arg,
		((cache_entry_t *) obj)->key,
		((cache_entry_t *) obj)->key_size);
}

tcam_cache_t *tcam_cache_create(int size, int key_size, int expiration_secs) {
  tcam_cache_t *cache =  malloc(sizeof(tcam_cache_t));
  cache->bitmap_size = ((size + 63) / 64);
  cache->size = cache->bitmap_size * size;
  cache->key_size = key_size;
  cache->expiration_secs = expiration_secs;
  cache->nb_entries = 0;
  cache->entries = calloc(cache->size, sizeof(cache_entry_t));
  tommy_hashtable_init(&cache->hashtable, 2 * cache->size);
  cache->bitmap_used = calloc(cache->bitmap_size, sizeof(unsigned long));
  pthread_mutex_init(&cache->lock, NULL);
  return cache;
};

void tcam_cache_destroy(tcam_cache_t *cache) {
  tommy_hashtable_done(&cache->hashtable);
  free(cache->entries);
  free(cache->bitmap_used);
  free(cache);
}

/* 0: found,
   1: not found, insert
   2: not found, do NOT insert
*/
int tcam_cache_lookup(tcam_cache_t *cache, uint8_t *key, void **data) {
  int acquired = pthread_mutex_trylock(&cache->lock);
  if(acquired){
    *data = NULL;
    return 2;
  }
  uint32_t hash = hashlittle(key, cache->key_size, 0);
  cache_entry_t *entry = tommy_hashtable_search(&cache->hashtable,
						cmp,
						key,
						hash);
  int result;
  if(entry) {
    entry->last_access = time(NULL);
    *data = entry->data;
    result = 0;
  }
  else {
    *data = NULL;
    result = 1;
  }
  pthread_mutex_unlock(&cache->lock);
  return result;
}

/* 0: inserted
   1: not enough space in cache
*/
int tcam_cache_insert(tcam_cache_t *cache, uint8_t *key, void *data) {
  pthread_mutex_lock(&cache->lock);
  if(cache->nb_entries == cache->size) {
    pthread_mutex_unlock(&cache->lock);
    return 1;
  }
  int i;
  cache_entry_t *entry = NULL;
  int trailing_ones;
  for(i = 0; i < cache->bitmap_size; i++) {
    trailing_ones = __builtin_ctzl(~cache->bitmap_used[i]);
    if(trailing_ones != sizeof(unsigned long)) {
      entry = &cache->entries[sizeof(unsigned long) * i + trailing_ones];
      cache->bitmap_used[i] |= (1 << trailing_ones); 
      break;
    }
  }
  
  entry->last_access = time(NULL);
  entry->key = key;
  entry->key_size = cache->key_size;
  entry->data = data;
  uint32_t hash = hashlittle(key, cache->key_size, 0);
  tommy_hashtable_insert(&cache->hashtable, &entry->node, entry, hash);
  cache->nb_entries++;
  pthread_mutex_unlock(&cache->lock);
  return 0;
}

int tcam_cache_purge(tcam_cache_t *cache) {
  int removed = 0;
  pthread_mutex_lock(&cache->lock);

  int i, j;
  cache_entry_t *entry;
  time_t now = time(NULL);
  unsigned long used;
  int trailing_zeros;
  for(i = 0; i < cache->bitmap_size; i++) {
    used = cache->bitmap_used[i];
    trailing_zeros = __builtin_ctzl(used);
    for(j = trailing_zeros; j < sizeof(unsigned long); j++) {
      if(used & (1 << j)){
	entry = &cache->entries[sizeof(unsigned long) * i + j];
	if ((now - entry->last_access) > cache->expiration_secs) {
	  cache->bitmap_used[i] &= ~(1 << j);
	  tommy_hashtable_remove_existing(&cache->hashtable, &entry->node);
	  removed++;
	}
      }
    }
  }
  
  cache->nb_entries -= removed;
  pthread_mutex_unlock(&cache->lock);
  
  return removed;
}

void tcam_cache_invalidate(tcam_cache_t *cache) {
  pthread_mutex_lock(&cache->lock);
  
  tommy_hashtable_empty(&cache->hashtable);
  memset(cache->bitmap_used, 0, cache->bitmap_size * sizeof(unsigned long));
  cache->nb_entries = 0;

  pthread_mutex_unlock(&cache->lock);
}
