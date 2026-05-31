#ifndef LRU_DYN_H
#define LRU_DYN_H

#include "config.h"

typedef struct{
    int ordem[MAX_SETS][MAX_WAYS];
    int ways;
} LRUDyn;

void init_lru_dyn(LRUDyn *lru, int ways);
void lru_dyn_on_hit(LRUDyn *lru, int set_idx, int way);
int lru_dyn_choose_victim(LRUDyn *lru, int set_idx);
void lru_dyn_on_fill(LRUDyn *lru, int set_idx, int way);

#endif
