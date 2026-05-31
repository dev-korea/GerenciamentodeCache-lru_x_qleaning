#include <stdio.h>
#include "sim_debug.h"
#include "config.h"

void print_cache_stats(const Cache *cache){
    int total;
    double hit_rate;

    total = cache->hits + cache->misses;
    hit_rate = (total > 0) ? (double)cache->hits / total * 100.0 : 0.0;

    printf("\n===== ESTATISTICAS =====\n");
    printf("Hits   : %d\n", cache->hits);
    printf("Misses : %d\n", cache->misses);
    printf("HitRate: %.2f%%\n", hit_rate);
}

void print_cache_dyn_stats(const CacheDyn *cache, const char *nome){
    int total;
    double hit_rate;

    total = cache->hits + cache->misses;
    hit_rate = (total > 0) ? (double)cache->hits / total * 100.0 : 0.0;

    printf("\n===== ESTATISTICAS %s =====\n", nome);
    printf("Hits   : %d\n", cache->hits);
    printf("Misses : %d\n", cache->misses);
    printf("HitRate: %.2f%%\n", hit_rate);
}

void print_qtable_state(const QLearning *ql, int set_idx, int tag){
    int reduced_tag;

    reduced_tag = tag % REDUCED_TAGS;

    printf("Q[set=%d][rtag=%d] = [%.3f, %.3f]\n",
        set_idx,
        reduced_tag,
        ql->q_table[set_idx][reduced_tag][0],
        ql->q_table[set_idx][reduced_tag][1]);
}

void print_qtable_state_fixed(const QLearningFixed *ql, int set_idx, int tag){
    int reduced_tag;

    reduced_tag = tag % REDUCED_TAGS;

    printf("Qfix[set=%d][rtag=%d] = [%d, %d]\n",
        set_idx,
        reduced_tag,
        ql->q_table[set_idx][reduced_tag][0],
        ql->q_table[set_idx][reduced_tag][1]);
}
