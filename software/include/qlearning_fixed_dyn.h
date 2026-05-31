#ifndef QLEARNING_FIXED_DYN_H
#define QLEARNING_FIXED_DYN_H

#include "config.h"

typedef struct{
    int q_table[MAX_SETS][REDUCED_TAGS][MAX_WAYS];

    int alpha;
    int gamma;
    int epsilon;
    int ways;
} QLearningFixedDyn;

void init_qlearning_fixed_dyn(QLearningFixedDyn *ql, int ways, int alpha, int gamma, int epsilon);
int qlfd_reduced_tag(int tag);
int qlfd_choose_action(QLearningFixedDyn *ql, int set_idx, int tag);
void qlfd_update(QLearningFixedDyn *ql, int set_idx, int tag, int way, int reward);
void qlfd_on_hit(QLearningFixedDyn *ql, int set_idx, int tag, int way);
void qlfd_on_miss_fill(QLearningFixedDyn *ql, int set_idx, int tag, int way);

#endif
