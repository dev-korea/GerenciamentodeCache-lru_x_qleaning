#ifndef LRU_H
#define LRU_H

#include "config.h"

typedef struct{
    int ordem[N_SETS][WAYS];
} LRU;

void init_lru(LRU *lru);
void lru_on_hit(LRU *lru, int set_idx, int way);
int lru_choose_victim(LRU *lru, int set_idx);
void lru_on_fill(LRU *lru, int set_idx, int way);

#endif
