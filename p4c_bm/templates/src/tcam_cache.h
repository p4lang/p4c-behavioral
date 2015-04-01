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

#ifndef _RMT_TCAM_CACHE_H
#define _RMT_TCAM_CACHE_H

#include <stdint.h>

typedef struct tcam_cache_s tcam_cache_t;

tcam_cache_t *tcam_cache_create(int size, int key_size, int expiration_secs);

void tcam_cache_destroy(tcam_cache_t *cache);

int tcam_cache_lookup(tcam_cache_t *cache, uint8_t *key, void **data);

int tcam_cache_insert(tcam_cache_t *cache, uint8_t *key, void *data);

int tcam_cache_purge(tcam_cache_t *cache);

void tcam_cache_invalidate(tcam_cache_t *cache);

#endif
