#ifndef CACHE_H
#define CACHE_H

#include "config.h"

typedef struct{
    int tag;
    int valid;
} CacheLine;

typedef struct{
    CacheLine linhas[WAYS];
} CacheSet;

typedef struct{
    CacheSet conjuntos[N_SETS];
    int hits;
    int misses;
} Cache;

void init_cache(Cache *cache);
void split_address(int address, int *set_idx, int *tag);
int find_hit(Cache *cache, int set_idx, int tag);
int find_empty_line(Cache *cache, int set_idx);
void fill_line(Cache *cache, int set_idx, int way, int tag);

#endif
