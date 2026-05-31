#include "qlearning_fixed.h"
#include "random_util.h"

void init_qlearning_fixed(QLearningFixed *ql, int alpha, int gamma, int epsilon){
    int i, j, k;

    ql->alpha = alpha;     /* ex: 100 = 0.10 * QL_SCALE */
    ql->gamma = gamma;     /* ex: 900 = 0.90 * QL_SCALE */
    ql->epsilon = epsilon; /* ex: 10  = 10%              */

    for(i = 0; i < N_SETS; i++){
        for(j = 0; j < REDUCED_TAGS; j++){
            for(k = 0; k < WAYS; k++){
                ql->q_table[i][j][k] = 0;
            }
        }
    }
}

int build_reduced_tag_fixed(int tag){
    return tag % REDUCED_TAGS;
}

int ql_fixed_choose_action(QLearningFixed *ql, int set_idx, int tag){
    int reduced_tag;
    int best_way;

    reduced_tag = build_reduced_tag_fixed(tag);

    if(rand_pct() < ql->epsilon){
        return rand_way(WAYS);
    }

    best_way = 0;

    if(ql->q_table[set_idx][reduced_tag][1] > ql->q_table[set_idx][reduced_tag][0]){
        best_way = 1;
    }

    return best_way;
}

void ql_fixed_update(QLearningFixed *ql, int set_idx, int tag, int action, int reward){
    int reduced_tag;
    int old_q;
    int best_next;
    int target;
    int diff;
    int new_q;

    reduced_tag = build_reduced_tag_fixed(tag);
    old_q = ql->q_table[set_idx][reduced_tag][action];

    best_next = ql->q_table[set_idx][reduced_tag][0];
    if(ql->q_table[set_idx][reduced_tag][1] > best_next){
        best_next = ql->q_table[set_idx][reduced_tag][1];
    }

    /* target = reward + gamma * best_next */
    target = reward + (ql->gamma * best_next) / QL_SCALE;

    /* diff = target - old_q */
    diff = target - old_q;

    /* new_q = old_q + alpha * diff */
    new_q = old_q + (ql->alpha * diff) / QL_SCALE;

    ql->q_table[set_idx][reduced_tag][action] = new_q;
}

void ql_fixed_on_hit(QLearningFixed *ql, int set_idx, int tag, int way){
    ql_fixed_update(ql, set_idx, tag, way, REWARD_HIT);
}

void ql_fixed_on_miss_fill(QLearningFixed *ql, int set_idx, int tag, int way){
    ql_fixed_update(ql, set_idx, tag, way, REWARD_MISS);
}
