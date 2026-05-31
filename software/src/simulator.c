#include <stdio.h>
#include "simulator.h"
#include "config.h"

/* =========================================================
   Cache unica com LRU
   ========================================================= */

Resultado run_lru(int acessos[], int n){
    Cache cache;
    LRU lru;
    Resultado r;

    int i;
    int set_idx, tag;
    int hit_way, empty_way, victim_way;

    init_cache(&cache);
    init_lru(&lru);

    for(i = 0; i < n; i++){
        split_address(acessos[i], &set_idx, &tag);

        hit_way = find_hit(&cache, set_idx, tag);

        if(hit_way != -1){
            cache.hits++;
            lru_on_hit(&lru, set_idx, hit_way);
        }else{
            cache.misses++;

            empty_way = find_empty_line(&cache, set_idx);

            if(empty_way != -1){
                fill_line(&cache, set_idx, empty_way, tag);
                lru_on_fill(&lru, set_idx, empty_way);
            }else{
                victim_way = lru_choose_victim(&lru, set_idx);
                fill_line(&cache, set_idx, victim_way, tag);
                lru_on_fill(&lru, set_idx, victim_way);
            }
        }
    }

    r.hits = cache.hits;
    r.misses = cache.misses;
    r.hit_rate = (cache.hits + cache.misses > 0)
        ? ((double)cache.hits / (cache.hits + cache.misses)) * 100.0
        : 0.0;

    return r;
}

/* =========================================================
   Cache unica com Q-Learning Float
   ========================================================= */

Resultado run_qlearning_float(int acessos[], int n){
    Cache cache;
    QLearning ql;
    Resultado r;

    int i;
    int set_idx, tag;
    int hit_way, empty_way, victim_way;

    init_cache(&cache);
    init_qlearning(&ql, 0.1f, 0.9f, 0.1f);

    for(i = 0; i < n; i++){
        split_address(acessos[i], &set_idx, &tag);

        hit_way = find_hit(&cache, set_idx, tag);

        if(hit_way != -1){
            cache.hits++;
            ql_on_hit(&ql, set_idx, tag, hit_way);
        }else{
            cache.misses++;

            empty_way = find_empty_line(&cache, set_idx);

            if(empty_way != -1){
                fill_line(&cache, set_idx, empty_way, tag);
                ql_on_miss_fill(&ql, set_idx, tag, empty_way);
            }else{
                victim_way = ql_choose_action(&ql, set_idx, tag);
                fill_line(&cache, set_idx, victim_way, tag);
                ql_on_miss_fill(&ql, set_idx, tag, victim_way);
            }
        }
    }

    r.hits = cache.hits;
    r.misses = cache.misses;
    r.hit_rate = (cache.hits + cache.misses > 0)
        ? ((double)cache.hits / (cache.hits + cache.misses)) * 100.0
        : 0.0;

    return r;
}

/* =========================================================
   Cache unica com Q-Learning Fixed-Point
   ========================================================= */

Resultado run_qlearning_fixed(int acessos[], int n){
    Cache cache;
    QLearningFixed ql;
    Resultado r;

    int i;
    int set_idx, tag;
    int hit_way, empty_way, victim_way;

    init_cache(&cache);
    init_qlearning_fixed(&ql, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);

    for(i = 0; i < n; i++){
        split_address(acessos[i], &set_idx, &tag);

        hit_way = find_hit(&cache, set_idx, tag);

        if(hit_way != -1){
            cache.hits++;
            ql_fixed_on_hit(&ql, set_idx, tag, hit_way);
        }else{
            cache.misses++;

            empty_way = find_empty_line(&cache, set_idx);

            if(empty_way != -1){
                fill_line(&cache, set_idx, empty_way, tag);
                ql_fixed_on_miss_fill(&ql, set_idx, tag, empty_way);
            }else{
                victim_way = ql_fixed_choose_action(&ql, set_idx, tag);
                fill_line(&cache, set_idx, victim_way, tag);
                ql_fixed_on_miss_fill(&ql, set_idx, tag, victim_way);
            }
        }
    }

    r.hits = cache.hits;
    r.misses = cache.misses;
    r.hit_rate = (cache.hits + cache.misses > 0)
        ? ((double)cache.hits / (cache.hits + cache.misses)) * 100.0
        : 0.0;

    return r;
}

/* =========================================================
   Auxiliares de insercao na hierarquia
   ========================================================= */

void insert_cache_lru_dyn(CacheDyn *cache, LRUDyn *lru, int address){
    int set_idx, tag;
    int hit_way, empty_way, victim_way;

    split_address_dyn(cache, address, &set_idx, &tag);

    hit_way = find_hit_dyn(cache, set_idx, tag);

    if(hit_way != -1){
        lru_dyn_on_hit(lru, set_idx, hit_way);
        return;
    }

    empty_way = find_empty_line_dyn(cache, set_idx);

    if(empty_way != -1){
        fill_line_dyn(cache, set_idx, empty_way, tag);
        lru_dyn_on_fill(lru, set_idx, empty_way);
    }else{
        victim_way = lru_dyn_choose_victim(lru, set_idx);
        fill_line_dyn(cache, set_idx, victim_way, tag);
        lru_dyn_on_fill(lru, set_idx, victim_way);
    }
}

void insert_l1_qlearning_fixed_dyn(CacheDyn *l1, QLearningFixed *ql, int address){
    int set_idx, tag;
    int hit_way, empty_way, victim_way;

    split_address_dyn(l1, address, &set_idx, &tag);

    hit_way = find_hit_dyn(l1, set_idx, tag);

    if(hit_way != -1){
        ql_fixed_on_hit(ql, set_idx, tag, hit_way);
        return;
    }

    empty_way = find_empty_line_dyn(l1, set_idx);

    if(empty_way != -1){
        fill_line_dyn(l1, set_idx, empty_way, tag);
        ql_fixed_on_miss_fill(ql, set_idx, tag, empty_way);
    }else{
        victim_way = ql_fixed_choose_action(ql, set_idx, tag);
        fill_line_dyn(l1, set_idx, victim_way, tag);
        ql_fixed_on_miss_fill(ql, set_idx, tag, victim_way);
    }
}

/* =========================================================
   Hierarquia L1 LRU + L2 LRU
   ========================================================= */

ResultadoHierarquia simular_lru_lru(int acessos[], int n, int verbose){
    CacheDyn l1;
    CacheDyn l2;
    LRUDyn lru_l1;
    LRUDyn lru_l2;
    ResultadoHierarquia r;

    int i;
    int address;
    int set_l1, tag_l1;
    int set_l2, tag_l2;
    int hit_l1;
    int hit_l2;

    r.l1_hits = 0;
    r.l1_misses = 0;
    r.l2_hits = 0;
    r.l2_misses = 0;

    init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
    init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);
    init_lru_dyn(&lru_l1, L1_WAYS);
    init_lru_dyn(&lru_l2, L2_WAYS);

    if(verbose){
        printf("\n=== HIERARQUIA: L1 LRU + L2 LRU ===\n");
    }

    for(i = 0; i < n; i++){
        address = acessos[i];

        split_address_dyn(&l1, address, &set_l1, &tag_l1);
        hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

        if(verbose){
            printf("\nAcesso 0x%04X", address);
        }

        if(hit_l1 != -1){
            r.l1_hits++;
            lru_dyn_on_hit(&lru_l1, set_l1, hit_l1);

            if(verbose){
                printf(" -> HIT L1");
            }

            continue;
        }

        r.l1_misses++;

        if(verbose){
            printf(" -> MISS L1");
        }

        split_address_dyn(&l2, address, &set_l2, &tag_l2);
        hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

        if(hit_l2 != -1){
            r.l2_hits++;
            lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

            if(verbose){
                printf(" | HIT L2");
            }

            insert_cache_lru_dyn(&l1, &lru_l1, address);
            continue;
        }

        r.l2_misses++;

        if(verbose){
            printf(" | MISS L2 | Busca memoria principal");
        }

        insert_cache_lru_dyn(&l2, &lru_l2, address);
        insert_cache_lru_dyn(&l1, &lru_l1, address);
    }

    return r;
}

/* =========================================================
   Hierarquia L1 Q-Learning Fixed + L2 LRU
   ========================================================= */

ResultadoHierarquia simular_ql_lru(int acessos[], int n, int verbose){
    CacheDyn l1;
    CacheDyn l2;
    QLearningFixed ql_l1;
    LRUDyn lru_l2;
    ResultadoHierarquia r;

    int i;
    int address;
    int set_l1, tag_l1;
    int set_l2, tag_l2;
    int hit_l1;
    int hit_l2;

    r.l1_hits = 0;
    r.l1_misses = 0;
    r.l2_hits = 0;
    r.l2_misses = 0;

    init_cache_dyn(&l1, L1_SIZE, L1_BLOCK, L1_WAYS);
    init_cache_dyn(&l2, L2_SIZE, L2_BLOCK, L2_WAYS);
    init_qlearning_fixed(&ql_l1, QL_DEFAULT_ALPHA, QL_DEFAULT_GAMMA, QL_DEFAULT_EPSILON);
    init_lru_dyn(&lru_l2, L2_WAYS);

    if(verbose){
        printf("\n=== HIERARQUIA: L1 Q-LEARNING FIXED + L2 LRU ===\n");
    }

    for(i = 0; i < n; i++){
        address = acessos[i];

        split_address_dyn(&l1, address, &set_l1, &tag_l1);
        hit_l1 = find_hit_dyn(&l1, set_l1, tag_l1);

        if(verbose){
            printf("\nAcesso 0x%04X", address);
        }

        if(hit_l1 != -1){
            r.l1_hits++;
            ql_fixed_on_hit(&ql_l1, set_l1, tag_l1, hit_l1);

            if(verbose){
                printf(" -> HIT L1");
            }

            continue;
        }

        r.l1_misses++;

        if(verbose){
            printf(" -> MISS L1");
        }

        split_address_dyn(&l2, address, &set_l2, &tag_l2);
        hit_l2 = find_hit_dyn(&l2, set_l2, tag_l2);

        if(hit_l2 != -1){
            r.l2_hits++;
            lru_dyn_on_hit(&lru_l2, set_l2, hit_l2);

            if(verbose){
                printf(" | HIT L2");
            }

            insert_l1_qlearning_fixed_dyn(&l1, &ql_l1, address);
            continue;
        }

        r.l2_misses++;

        if(verbose){
            printf(" | MISS L2 | Busca memoria principal");
        }

        insert_cache_lru_dyn(&l2, &lru_l2, address);
        insert_l1_qlearning_fixed_dyn(&l1, &ql_l1, address);
    }

    return r;
}
