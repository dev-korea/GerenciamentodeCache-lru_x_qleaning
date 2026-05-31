#ifndef SIM_TYPES_H
#define SIM_TYPES_H

/* Resultado de simulacao de cache unica */
typedef struct{
    int hits;
    int misses;
    double hit_rate;
} Resultado;

/* Resultado de simulacao de hierarquia L1 + L2 */
typedef struct{
    int l1_hits;
    int l1_misses;
    int l2_hits;
    int l2_misses;
} ResultadoHierarquia;

#endif
