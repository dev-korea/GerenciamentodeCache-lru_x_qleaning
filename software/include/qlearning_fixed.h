#ifndef QLEARNING_FIXED_H
#define QLEARNING_FIXED_H

#include "config.h"

typedef struct{
    int q_table[N_SETS][REDUCED_TAGS][WAYS];
    int alpha;
    int gamma;
    int epsilon;
} QLearningFixed;

void init_qlearning_fixed(QLearningFixed *ql, int alpha, int gamma, int epsilon);
int build_reduced_tag_fixed(int tag);
int ql_fixed_choose_action(QLearningFixed *ql, int set_idx, int tag);
void ql_fixed_update(QLearningFixed *ql, int set_idx, int tag, int action, int reward);
void ql_fixed_on_hit(QLearningFixed *ql, int set_idx, int tag, int way);
void ql_fixed_on_miss_fill(QLearningFixed *ql, int set_idx, int tag, int way);

#endif
