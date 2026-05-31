#ifndef CONFIG_H
#define CONFIG_H

/* =========================================================
   Cache L1 estatica (modulo cache.h / lru.h / qlearning*.h)
   ========================================================= */
#define CACHE_SIZE_BYTES  4096
#define BLOCK_SIZE        32
#define WAYS              2
#define N_SETS            (CACHE_SIZE_BYTES / (BLOCK_SIZE * WAYS))

/* =========================================================
   Hierarquia L1 + L2
   ========================================================= */
#define L1_SIZE   4096
#define L1_BLOCK  32
#define L1_WAYS   2

#define L2_SIZE   32768
#define L2_BLOCK  64
#define L2_WAYS   8

/* =========================================================
   Limites da cache dinamica (arrays estaticos internos)
   ========================================================= */
#define MAX_SETS  64
#define MAX_WAYS  8

/* =========================================================
   Capacidade maxima de trace em memoria
   ========================================================= */
#define MAX_TRACE 4096

/* =========================================================
   Q-Learning: escala e hiper-parametros padrao
   =========================================================
   QL_SCALE e uma potencia de 2 recomendada para Verilog
   (divisao vira shift de bits). Valor atual: 1000.
   Para migrar para hardware, trocar para 1024 e ajustar
   os limites de representacao dos Q-values.
   ========================================================= */
#define REDUCED_TAGS        16      /* num. de tags reduzidas (tag % 16 = 4 LSBs) */
#define QL_SCALE            1000    /* fator de escala fixed-point                */
#define REWARD_HIT          (QL_SCALE)
#define REWARD_MISS         (-QL_SCALE)

#define QL_DEFAULT_ALPHA    100     /* 0.10 * QL_SCALE */
#define QL_DEFAULT_GAMMA    900     /* 0.90 * QL_SCALE */
#define QL_DEFAULT_EPSILON  10      /* 10%             */

#endif
