#ifndef QLEARNING_H
#define QLEARNING_H

#include "config.h"

typedef struct{
    float q_table[N_SETS][REDUCED_TAGS][WAYS];
    float alpha;
    float gamma;
    float epsilon;
} QLearning;

void init_qlearning(QLearning *ql, float alpha, float gamma, float epsilon);
int build_reduced_tag(int tag);
int ql_choose_action(QLearning *ql, int set_idx, int tag);
void ql_update(QLearning *ql, int set_idx, int tag, int action, float reward);
void ql_on_hit(QLearning *ql, int set_idx, int tag, int way);
void ql_on_miss_fill(QLearning *ql, int set_idx, int tag, int way);

#endif
