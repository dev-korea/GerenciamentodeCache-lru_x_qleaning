# Cache L1/L2 — Simulação em C (LRU × Q-Learning)

Simulador em C de hierarquia de cache de dois níveis com três políticas de substituição de linhas: **LRU**, **Q-Learning ponto flutuante** e **Q-Learning ponto fixo**. O objetivo é validar o comportamento do Q-Learning em software antes da síntese em Verilog.

---

## Parâmetros de hardware simulados

| Parâmetro | L1 | L2 |
|-----------|----|----|
| Tamanho | 4096 B | 32768 B |
| Tamanho do bloco | 32 B | 64 B |
| Número de vias | 2 | 8 |
| Número de conjuntos | 64 | 64 |

### Q-Learning (ponto fixo)

| Hiperparâmetro | Valor | Significado |
|---------------|-------|-------------|
| `QL_SCALE` | 1000 | Fator de escala (usar 1024 no hardware) |
| `alpha` | 100 | Taxa de aprendizado = 0,10 |
| `gamma` | 900 | Fator de desconto = 0,90 |
| `epsilon` | 10 | Taxa de exploração = 10% |
| `REDUCED_TAGS` | 16 | Dimensão de tag na Q-table (4 LSBs) |
| `REWARD_HIT` | +1000 | Recompensa por acerto |
| `REWARD_MISS` | −1000 | Penalidade por falta |

---

## Estrutura de arquivos

```
software/
├── include/
│   ├── config.h              # Parâmetros globais (tamanhos, hiperparâmetros QL)
│   ├── cache.h               # Cache estática (L1 fixa: 64 sets × 2 vias)
│   ├── cache_dyn.h           # Cache parametrizável em tempo de execução
│   ├── lru.h                 # LRU estático
│   ├── lru_dyn.h             # LRU parametrizável
│   ├── qlearning.h           # Q-Learning ponto flutuante
│   ├── qlearning_fixed.h     # Q-Learning ponto fixo (inteiro)
│   ├── qlearning_fixed_dyn.h # Q-Learning ponto fixo parametrizável
│   ├── simulator.h           # Motor de simulação (ponto de partida para Verilog)
│   ├── sim_types.h           # Tipos de resultado (Resultado, ResultadoHierarquia)
│   ├── random_util.h         # rand_pct(), rand_way() — substituir por LFSR no HW
│   └── sim_debug.h           # Funções de exibição (não implementar em hardware)
└── src/
    ├── cache.c / cache_dyn.c
    ├── lru.c / lru_dyn.c
    ├── qlearning.c
    ├── qlearning_fixed.c / qlearning_fixed_dyn.c
    ├── simulator.c                    # Núcleo: run_lru, run_qlearning_*, simular_*
    ├── random_util.c
    ├── sim_debug.c
    ├── main.c                         # Menu comparativo (cache única)
    ├── main_hierarchy_lru.c           # Hierarquia L1 LRU + L2 LRU
    ├── main_hierarchy_ql_l1.c         # Hierarquia L1 Q-Learning + L2 LRU
    ├── main_benchmarks.c
    ├── main_benchmarks_ql_l2.c
    ├── main_parametros_l1.c
    └── main_validacao_50_conflito.c
```

---

## Arquitetura de módulos

```
simulator.c  ──────────────────────────────────────────────
  run_lru()              simula cache única com LRU
  run_qlearning_float()  simula cache única com QL float
  run_qlearning_fixed()  simula cache única com QL ponto fixo
  simular_lru_lru()      hierarquia L1 LRU + L2 LRU
  simular_ql_lru()       hierarquia L1 QL fixo + L2 LRU
       │
       ├── cache.c / cache_dyn.c  →  split_address, find_hit,
       │                              find_empty_line, fill_line
       ├── lru.c / lru_dyn.c      →  init_lru, on_hit, on_fill,
       │                              choose_victim
       ├── qlearning_fixed.c      →  choose_action (ε-greedy),
       │                              ql_update (ponto fixo)
       └── random_util.c          →  rand_pct(), rand_way()
```

---

## Algoritmo de acesso à cache

```
ACESSO(endereço):
  (set, tag) ← split_address(endereço)
  via        ← find_hit(cache, set, tag)

  SE via != -1:                          // HIT
    hits++
    lru_on_hit(lru, set, via)

  SENÃO:                                 // MISS
    misses++
    vazia ← find_empty_line(cache, set)

    SE vazia != -1:
      fill_line(cache, set, vazia, tag)
      lru_on_fill(lru, set, vazia)
    SENÃO:
      vítima ← lru_choose_victim(lru, set)   // ou ql_choose_action(ql, set, tag)
      fill_line(cache, set, vítima, tag)
      lru_on_fill(lru, set, vítima)          // ou ql_on_miss_fill(...)
```

---

## Algoritmo Q-Learning (ponto fixo)

```c
// Atualização da Q-table — equivalente a ql_update() em qlearning_fixed.c
target = reward + (gamma * best_next) / QL_SCALE;
diff   = target - old_q;
new_q  = old_q + (alpha * diff) / QL_SCALE;
```

A Q-table tem dimensões `[N_SETS][REDUCED_TAGS][WAYS]`:
- `N_SETS = 64`: estado por conjunto
- `REDUCED_TAGS = 16`: tag comprimida (`tag % 16` → 4 LSBs)
- `WAYS = 2` (L1) ou `8` (L2): ações disponíveis

---

## Resultados de validação

| Trace | LRU | QL Float | QL Ponto Fixo |
|-------|-----|----------|---------------|
| Boa localidade | 75,00% | 75,00% | 75,00% |
| Conflito forte | 0,00% | 11,11% | 11,11% |
| Misto | 25,00% | 41,67% | 41,67% |

A versão ponto fixo apresentou resultados **equivalentes** à versão em ponto flutuante, validando a aritmética inteira para implementação em hardware.

---

## Notas para a transição para Verilog

| C | Verilog |
|---|---------|
| `address / BLOCK_SIZE` | `address >> 5` (L1) / `>> 6` (L2) |
| `block_addr % N_SETS` | `address[10:5]` (L1) / `address[11:6]` (L2) |
| `tag % 16` | `tag[3:0]` |
| `/ QL_SCALE` (= 1000) | `>> 10` (usar `QL_SCALE = 1024` no HW) |
| `rand_pct()`, `rand_way()` | LFSR de 8 bits |
| Funções `print_*` | Não implementar em hardware |
| Q-table `int[64][16][2]` | BRAM 8 KB (L1) |
| Q-table `int[64][16][8]` | BRAM 32 KB (L2) |
