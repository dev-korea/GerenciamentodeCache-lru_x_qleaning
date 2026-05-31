#include "qlearning.h"
#include "random_util.h"

void init_qlearning(QLearning *ql, float alpha, float gamma, float epsilon){
    int i, j, k;

    ql->alpha = alpha;
    ql->gamma = gamma;
    ql->epsilon = epsilon;

    for(i = 0; i < N_SETS; i++){
        for(j = 0; j < REDUCED_TAGS; j++){
            for(k = 0; k < WAYS; k++){
                ql->q_table[i][j][k] = 0.0f;
            }
        }
    }
}

int build_reduced_tag(int tag){
    return tag % REDUCED_TAGS;
}

int ql_choose_action(QLearning *ql, int set_idx, int tag){
    int reduced_tag;
    int best_way;

    reduced_tag = build_reduced_tag(tag);

    if(rand_pct() < (int)(ql->epsilon * 100)){
        return rand_way(WAYS);
    }

    best_way = 0;

    if(ql->q_table[set_idx][reduced_tag][1] > ql->q_table[set_idx][reduced_tag][0]){
        best_way = 1;
    }

    return best_way;
}

void ql_update(QLearning *ql, int set_idx, int tag, int action, float reward){
    int reduced_tag;
    float old_q;
    float best_next;
    float new_q;

    reduced_tag = build_reduced_tag(tag);
    old_q = ql->q_table[set_idx][reduced_tag][action];

    best_next = ql->q_table[set_idx][reduced_tag][0];
    if(ql->q_table[set_idx][reduced_tag][1] > best_next){
        best_next = ql->q_table[set_idx][reduced_tag][1];
    }

    new_q = old_q + ql->alpha * (reward + ql->gamma * best_next - old_q);
    ql->q_table[set_idx][reduced_tag][action] = new_q;
}

void ql_on_hit(QLearning *ql, int set_idx, int tag, int way){
    ql_update(ql, set_idx, tag, way, 1.0f);
}

void ql_on_miss_fill(QLearning *ql, int set_idx, int tag, int way){
    ql_update(ql, set_idx, tag, way, -1.0f);
}
