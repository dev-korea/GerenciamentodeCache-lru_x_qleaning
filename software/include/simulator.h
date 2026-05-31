#ifndef SIMULATOR_H
#define SIMULATOR_H

/*
 * Motor de simulacao: lacos de acesso a cache desacoplados
 * da logica de exibicao e dos geradores de trace.
 *
 * Estas funcoes representam o comportamento do hardware —
 * sao o ponto de partida para a traducao em Verilog.
 */

#include "sim_types.h"
#include "cache.h"
#include "lru.h"
#include "qlearning.h"
#include "qlearning_fixed.h"
#include "cache_dyn.h"
#include "lru_dyn.h"

/* --- Cache unica --- */
Resultado run_lru(int acessos[], int n);
Resultado run_qlearning_float(int acessos[], int n);
Resultado run_qlearning_fixed(int acessos[], int n);

/* --- Auxiliares para hierarquia --- */
void insert_cache_lru_dyn(CacheDyn *cache, LRUDyn *lru, int address);
void insert_l1_qlearning_fixed_dyn(CacheDyn *l1, QLearningFixed *ql, int address);

/* --- Hierarquia L1 + L2 --- */
ResultadoHierarquia simular_lru_lru(int acessos[], int n, int verbose);
ResultadoHierarquia simular_ql_lru(int acessos[], int n, int verbose);

#endif
