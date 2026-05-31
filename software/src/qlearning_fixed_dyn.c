#include "qlearning_fixed_dyn.h"
#include "random_util.h"

void init_qlearning_fixed_dyn(QLearningFixedDyn *ql, int ways, int alpha, int gamma, int epsilon){
    int i, j, k;

    ql->ways = ways;
    ql->alpha = alpha;
    ql->gamma = gamma;
    ql->epsilon = epsilon;

    for(i = 0; i < MAX_SETS; i++){
        for(j = 0; j < REDUCED_TAGS; j++){
            for(k = 0; k < MAX_WAYS; k++){
                ql->q_table[i][j][k] = 0;
            }
        }
    }
}

int qlfd_reduced_tag(int tag){
    return tag % REDUCED_TAGS;
}

int qlfd_choose_action(QLearningFixedDyn *ql, int set_idx, int tag){
    int rtag;
    int i;
    int best_way;
    int best_q;

    rtag = qlfd_reduced_tag(tag);

    if(rand_pct() < ql->epsilon){
        return rand_way(ql->ways);
    }

    best_way = 0;
    best_q = ql->q_table[set_idx][rtag][0];

    for(i = 1; i < ql->ways; i++){
        if(ql->q_table[set_idx][rtag][i] > best_q){
            best_q = ql->q_table[set_idx][rtag][i];
            best_way = i;
        }
    }

    return best_way;
}

void qlfd_update(QLearningFixedDyn *ql, int set_idx, int tag, int way, int reward){
    int rtag;
    int i;
    int old_q;
    int best_next;
    int target;
    int diff;
    int new_q;

    rtag = qlfd_reduced_tag(tag);

    old_q = ql->q_table[set_idx][rtag][way];

    best_next = ql->q_table[set_idx][rtag][0];

    for(i = 1; i < ql->ways; i++){
        if(ql->q_table[set_idx][rtag][i] > best_next){
            best_next = ql->q_table[set_idx][rtag][i];
        }
    }

    target = reward + (ql->gamma * best_next) / QL_SCALE;
    diff = target - old_q;
    new_q = old_q + (ql->alpha * diff) / QL_SCALE;

    ql->q_table[set_idx][rtag][way] = new_q;
}

void qlfd_on_hit(QLearningFixedDyn *ql, int set_idx, int tag, int way){
    qlfd_update(ql, set_idx, tag, way, REWARD_HIT);
}

void qlfd_on_miss_fill(QLearningFixedDyn *ql, int set_idx, int tag, int way){
    qlfd_update(ql, set_idx, tag, way, REWARD_MISS);
}
