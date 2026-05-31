#ifndef SIM_DEBUG_H
#define SIM_DEBUG_H

/*
 * Funcoes de exibicao para uso exclusivo na simulacao em software.
 * NAO faz parte do hardware sintetizavel — nao incluir em modulos
 * que serao traduzidos para Verilog.
 */

#include "cache.h"
#include "cache_dyn.h"
#include "qlearning.h"
#include "qlearning_fixed.h"

void print_cache_stats(const Cache *cache);
void print_cache_dyn_stats(const CacheDyn *cache, const char *nome);
void print_qtable_state(const QLearning *ql, int set_idx, int tag);
void print_qtable_state_fixed(const QLearningFixed *ql, int set_idx, int tag);

#endif
