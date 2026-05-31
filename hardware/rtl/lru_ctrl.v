// Controlador de política LRU (Least Recently Used) parametrizável.
// Equivalente a lru.c / lru_dyn.c: init_lru, lru_on_hit, lru_on_fill,
// lru_choose_victim.
//
// Mantém por conjunto um vetor de ordenação (permutação das vias):
//   slot[0] = via LRU (vítima)   slot[WAYS-1] = via MRU
//
// No acesso (hit ou fill): a via acessada vai para slot[WAYS-1].
// O restante desloca uma posição para a esquerda (slot k ← slot k+1).

module lru_ctrl #(
    parameter N_SETS = 64,
    parameter WAYS   = 2
)(
    input                        clk,
    input                        rst,

    // --- Atualização (hit ou fill) ---
    input                        upd_en,
    input  [$clog2(N_SETS)-1:0]  upd_set,   // conjunto acessado
    input  [$clog2(WAYS)-1:0]    upd_way,   // via acessada

    // --- Consulta de vítima ---
    input  [$clog2(N_SETS)-1:0]  rd_set,    // conjunto a consultar
    output [$clog2(WAYS)-1:0]    victim_way // via LRU (slot[0])
);
    localparam WAY_BITS  = $clog2(WAYS);
    localparam SLOT_BITS = WAYS * WAY_BITS;

    // Vetor de ordenação por conjunto, empacotado:
    //   ordem_vec[s][k*WAY_BITS +: WAY_BITS] = conteúdo do slot k do conjunto s
    reg [SLOT_BITS-1:0] ordem_vec [0:N_SETS-1];

    // Posição atual de upd_way no vetor de upd_set (combinacional)
    reg [WAY_BITS-1:0] found_pos;
    integer k;

    always @(*) begin
        found_pos = {WAY_BITS{1'b0}};
        for (k = 0; k < WAYS; k = k + 1) begin
            if (ordem_vec[upd_set][k*WAY_BITS +: WAY_BITS] == upd_way)
                found_pos = k;  // trunca para WAY_BITS
        end
    end

    // Reset e atualização síncrona
    integer s, m;

    always @(posedge clk) begin
        if (rst) begin
            // Estado inicial: slot[k] = via k (slot[0]=via LRU, slot[WAYS-1]=via MRU)
            for (s = 0; s < N_SETS; s = s + 1)
                for (k = 0; k < WAYS; k = k + 1)
                    ordem_vec[s][k*WAY_BITS +: WAY_BITS] <= k;
        end else if (upd_en) begin
            // Slots abaixo de found_pos: inalterados (registradores mantêm valor)
            // Slots [found_pos .. WAYS-2]: deslocam — cada um recebe o próximo
            for (m = 0; m < WAYS - 1; m = m + 1) begin
                if (m >= found_pos)
                    ordem_vec[upd_set][m*WAY_BITS +: WAY_BITS] <=
                        ordem_vec[upd_set][(m+1)*WAY_BITS +: WAY_BITS];
            end
            // Slot WAYS-1 (MRU) recebe a via acessada
            ordem_vec[upd_set][(WAYS-1)*WAY_BITS +: WAY_BITS] <= upd_way;
        end
    end

    // Vítima = slot[0] do conjunto consultado (combinacional)
    assign victim_way = ordem_vec[rd_set][WAY_BITS-1:0];

endmodule
