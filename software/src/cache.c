#include "../include/cache.h"

void init_cache(Cache *cache){
    int i, j;

    cache->hits = 0;
    cache->misses = 0;

    for(i = 0; i < N_SETS; i++){
        for(j = 0; j < WAYS; j++){
            cache->conjuntos[i].linhas[j].valid = 0;
            cache->conjuntos[i].linhas[j].tag = 0;
        }
    }
}

void split_address(int address, int *set_idx, int *tag){
    int block_addr;

    block_addr = address / BLOCK_SIZE;
    *set_idx = block_addr % N_SETS;
    *tag = block_addr / N_SETS;
}

int find_hit(Cache *cache, int set_idx, int tag){
    int i;

    for(i = 0; i < WAYS; i++){
        if(cache->conjuntos[set_idx].linhas[i].valid &&
           cache->conjuntos[set_idx].linhas[i].tag == tag){
            return i;
        }
    }

    return -1;
}

int find_empty_line(Cache *cache, int set_idx){
    int i;

    for(i = 0; i < WAYS; i++){
        if(cache->conjuntos[set_idx].linhas[i].valid == 0){
            return i;
        }
    }

    return -1;
}

void fill_line(Cache *cache, int set_idx, int way, int tag){
    cache->conjuntos[set_idx].linhas[way].tag = tag;
    cache->conjuntos[set_idx].linhas[way].valid = 1;
}
