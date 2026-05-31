#ifndef CACHE_DYN_H
#define CACHE_DYN_H

#include "config.h"

typedef struct{
    int tag;
    int valid;
} CacheLineDyn;

typedef struct{
    CacheLineDyn linhas[MAX_WAYS];
} CacheSetDyn;

typedef struct{
    CacheSetDyn conjuntos[MAX_SETS];

    int size_bytes;
    int block_size;
    int ways;
    int n_sets;

    int hits;
    int misses;
} CacheDyn;

void init_cache_dyn(CacheDyn *cache, int size_bytes, int block_size, int ways);
void split_address_dyn(CacheDyn *cache, int address, int *set_idx, int *tag);
int find_hit_dyn(CacheDyn *cache, int set_idx, int tag);
int find_empty_line_dyn(CacheDyn *cache, int set_idx);
void fill_line_dyn(CacheDyn *cache, int set_idx, int way, int tag);

#endif
