#ifndef RANDOM_UTIL_H
#define RANDOM_UTIL_H

/*
 * Abstrai as chamadas a rand() da stdlib.
 *
 * Em Verilog, substituir a implementacao por um LFSR
 * (Linear Feedback Shift Register) sem alterar os
 * pontos de chamada nos modulos de politica.
 */

/* Retorna inteiro uniforme em [0, 99] */
int rand_pct(void);

/* Retorna inteiro uniforme em [0, n-1] */
int rand_way(int n);

#endif
