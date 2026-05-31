# Cache L1/L2 — Implementação em Verilog (LRU × Q-Learning)

Implementação RTL em Verilog da hierarquia de cache de dois níveis com políticas LRU e Q-Learning ponto fixo. Os módulos são derivados diretamente da simulação em C (`../software/`) e seguem os mesmos parâmetros de hardware.

---

## Parâmetros de hardware

| Parâmetro | L1 | L2 |
|-----------|----|----|
| Tamanho | 4096 B | 32768 B |
| Tamanho do bloco | 32 B | 64 B |
| Número de vias | 2 | 8 |
| Número de conjuntos | 64 | 64 |
| `BLOCK_BITS` (log2 bloco) | 5 | 6 |
| `SET_BITS` (log2 sets) | 6 | 6 |
| `TAG_WIDTH` | 21 bits | 20 bits |

### Decomposição de endereço (32 bits)

```
L1:  [ tag: 21b | set: 6b | offset: 5b ]
      address[31:11]  [10:5]   [4:0]

L2:  [ tag: 20b | set: 6b | offset: 6b ]
      address[31:12]  [11:6]   [5:0]
```

### Q-Learning (ponto fixo)

| Parâmetro | Valor HW | Observação |
|-----------|----------|------------|
| `QL_SCALE` | **1024** | Divisão via `>> 10` (potência de 2) |
| `REDUCED_TAG` | `tag[3:0]` | 4 LSBs da tag (equivale a `tag % 16`) |
| Q-table L1 | 64 × 16 × 2 × 32 b | ≈ 8 KB de BRAM |
| Q-table L2 | 64 × 16 × 8 × 32 b | ≈ 32 KB de BRAM |
| Aleatoriedade | LFSR 8 bits | Substitui `rand_pct()` e `rand_way()` do C |

---

## Estrutura de arquivos

```
hardware/
├── rtl/                        # Módulos sintetizáveis
│   ├── address_splitter.v      # [✓] Decomposição de endereço
│   ├── cache_array.v           # [✓] Array de cache (valid, tag, hit, fill)
│   ├── lru_ctrl.v              # [✓] Controlador LRU parametrizável
│   ├── q_table.v               # [ ] BRAM da Q-table
│   ├── ql_update.v             # [ ] ALU de atualização Q-Learning
│   ├── ql_policy.v             # [ ] Política ε-greedy + argmax
│   ├── victim_select.v         # [ ] Mux: LRU vs QL
│   ├── cache_fsm.v             # [ ] FSM principal de acesso
│   └── hierarchy_ctrl.v        # [ ] Controlador de hierarquia L1 + L2
└── tb/                         # Testbenches (a criar)
```

---

## Módulos implementados

### `address_splitter.v`

Módulo combinacional puro. Equivale a `split_address()` / `split_address_dyn()` em C.

```
Parâmetros: ADDR_WIDTH, BLOCK_BITS, SET_BITS
Entradas:   address[ADDR_WIDTH-1:0]
Saídas:     set_idx[SET_BITS-1:0]
            tag[ADDR_WIDTH-BLOCK_BITS-SET_BITS-1:0]
```

Instanciar duas vezes: uma para L1 (`BLOCK_BITS=5`) e uma para L2 (`BLOCK_BITS=6`).

---

### `cache_array.v`

Array de valid/tag com detecção de hit e de via livre. Equivale a `find_hit()`, `find_empty_line()` e `fill_line()` em C.

```
Parâmetros: N_SETS=64, WAYS=2|8, TAG_WIDTH
Entradas:   clk, rst
            set_addr, rd_tag        (leitura — combinacional)
            wr_en, wr_set, wr_way, wr_tag  (fill — síncrono)
Saídas:     hit, hit_way            (acerto e via correspondente)
            has_empty, empty_way    (via livre disponível)
```

- Hit e empty são calculados com comparadores paralelos (todas as vias ao mesmo tempo).
- Reset síncrono zera todos os bits `valid`.

---

### `lru_ctrl.v`

Registrador de ordenação LRU por conjunto. Equivale a `lru_on_hit()`, `lru_on_fill()` e `lru_choose_victim()` em C.

```
Parâmetros: N_SETS=64, WAYS=2|8
Entradas:   clk, rst
            upd_en, upd_set, upd_way   (acesso — síncrono)
            rd_set                      (consulta de vítima)
Saídas:     victim_way                  (via LRU — combinacional)
```

Armazena `ordem_vec[set]`: vetor empacotado onde `slot[0]` = LRU (vítima) e `slot[WAYS-1]` = MRU. No acesso:
1. Localiza a posição atual da via acessada (combinacional).
2. Desloca os slots superiores uma posição para baixo.
3. Coloca a via no topo (MRU).

---

## Módulos planejados

### `q_table.v`
BRAM dual-port indexada por `(set, reduced_tag, way)`. Suporte a leitura e escrita síncronas, com porta de leitura antecipada para o argmax.

### `ql_update.v`
ALU que implementa a equação de atualização Q-Learning:
```
target = reward + (gamma × best_next) >> 10
diff   = target − old_q
new_q  = old_q + (alpha × diff) >> 10
```

### `ql_policy.v`
Seleção de ação ε-greedy: LFSR de 8 bits gera aleatoriedade; se `lfsr[7:1] < epsilon` → exploração (via aleatória); caso contrário → `argmax(Q[set][rtag][*])`.

### `victim_select.v`
Mux 1-bit que escolhe entre `victim_way` (saída do `lru_ctrl`) e `ql_action` (saída do `ql_policy`) conforme a política ativa.

### `cache_fsm.v`
Máquina de estados principal:

```
IDLE → CHECK_HIT → { HIT:  UPDATE_LRU  → IDLE
                   { MISS: FIND_VICTIM → FILL → QL_UPDATE → IDLE
```

### `hierarchy_ctrl.v`
Orquestra os dois níveis: em miss de L1, consulta L2; em hit de L2, faz fill em L1; em miss de L2, faz fill em ambos.

---

## Correspondência C → Verilog

| Módulo C | Módulo Verilog | Observação |
|----------|---------------|------------|
| `split_address()` | `address_splitter.v` | Puramente combinacional |
| `find_hit()` | `cache_array.v` | Comparadores paralelos |
| `find_empty_line()` | `cache_array.v` | Detecção simultânea |
| `fill_line()` | `cache_array.v` | Escrita síncrona |
| `lru_on_hit/fill()` | `lru_ctrl.v` | Shift de ordenação |
| `lru_choose_victim()` | `lru_ctrl.v` | Leitura de slot[0] |
| `ql_update()` | `ql_update.v` | ALU com shifts |
| `ql_choose_action()` | `ql_policy.v` | LFSR + argmax |
| `rand_pct/way()` | `ql_policy.v` | LFSR 8 bits |
| `print_*()` | — | Não implementar |
