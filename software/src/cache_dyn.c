#include <stdio.h>
#include "cache_dyn.h"

void init_cache_dyn(CacheDyn *cache, int size_bytes, int block_size, int ways){
    int i, j;

    cache->size_bytes = size_bytes;
    cache->block_size = block_size;
    cache->ways = ways;
    cache->n_sets = size_bytes / (block_size * ways);

    cache->hits = 0;
    cache->misses = 0;

    for(i = 0; i < MAX_SETS; i++){
        for(j = 0; j < MAX_WAYS; j++){
            cache->conjuntos[i].linhas[j].valid = 0;
            cache->conjuntos[i].linhas[j].tag = 0;
        }
    }
}

void split_address_dyn(CacheDyn *cache, int address, int *set_idx, int *tag){
    int block_addr;

    block_addr = address / cache->block_size;
    *set_idx = block_addr % cache->n_sets;
    *tag = block_addr / cache->n_sets;
}

int find_hit_dyn(CacheDyn *cache, int set_idx, int tag){
    int i;

    for(i = 0; i < cache->ways; i++){
        if(cache->conjuntos[set_idx].linhas[i].valid &&
           cache->conjuntos[set_idx].linhas[i].tag == tag){
            return i;
        }
    }

    return -1;
}

int find_empty_line_dyn(CacheDyn *cache, int set_idx){
    int i;

    for(i = 0; i < cache->ways; i++){
        if(cache->conjuntos[set_idx].linhas[i].valid == 0){
            return i;
        }
    }

    return -1;
}

void fill_line_dyn(CacheDyn *cache, int set_idx, int way, int tag){
    cache->conjuntos[set_idx].linhas[way].tag = tag;
    cache->conjuntos[set_idx].linhas[way].valid = 1;
}
