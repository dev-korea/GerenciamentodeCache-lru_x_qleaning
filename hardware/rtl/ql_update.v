// ALU de atualização Q-Learning (ponto fixo, puramente combinacional).
// Equivalente a ql_fixed_update() / qlfd_update() em C.
//
// Equação implementada (QL_SCALE = 2^SCALE_BITS = 1024):
//   best_q  = max(q_vals[0 .. WAYS-1])
//   target  = reward + (GAMMA * best_q) >> SCALE_BITS
//   diff    = target - old_q
//   new_q   = old_q + (ALPHA * diff) >> SCALE_BITS
//
// Nota: QL_SCALE=1024 (hardware) vs 1000 (simulação C). Diferença < 2,5%,
// aceitável para o comportamento de aprendizado.
//
// Multiplicações de 32×32 bits produzem intermediário de 64 bits — o FPGA
// mapeia automaticamente para blocos DSP.

module ql_update #(
    parameter WAYS         = 2,
    parameter DATA_WIDTH   = 32,
    parameter SCALE_BITS   = 10,     // 2^10 = 1024
    parameter ALPHA        = 100,    // taxa de aprendizado × 1024 ≈ 0.098
    parameter GAMMA        = 900,    // fator de desconto  × 1024 ≈ 0.879
    parameter REWARD_HIT   = 1024,
    parameter REWARD_MISS  = -1024
)(
    // Entradas
    input  [WAYS*DATA_WIDTH-1:0]  q_vals,    // {q[WAYS-1],...,q[1],q[0]} da q_table
    input  [$clog2(WAYS)-1:0]     action,    // via selecionada para atualizar
    input                         is_hit,    // 1 = acerto, 0 = falta

    // Saídas
    output [DATA_WIDTH-1:0]       new_q,     // Q-value atualizado para action
    output [$clog2(WAYS)-1:0]     best_way   // argmax (para uso pelo ql_policy)
);
    localparam WAY_BITS = $clog2(WAYS);

    localparam signed [DATA_WIDTH-1:0] GAMMA_S      = GAMMA;
    localparam signed [DATA_WIDTH-1:0] ALPHA_S      = ALPHA;
    localparam signed [DATA_WIDTH-1:0] REWARD_HIT_S = REWARD_HIT;
    localparam signed [DATA_WIDTH-1:0] REWARD_MISS_S = REWARD_MISS;

    // --- old_q: Q-value da via selecionada (mux por action) ---
    reg signed [DATA_WIDTH-1:0] old_q;
    integer w;
    always @(*) begin
        old_q = {DATA_WIDTH{1'b0}};
        for (w = 0; w < WAYS; w = w + 1)
            if (action == w)
                old_q = $signed(q_vals[w*DATA_WIDTH +: DATA_WIDTH]);
    end

    // --- Argmax: best_q e best_way sobre todas as vias ---
    // Equivalente ao loop "best_next" em qlfd_update() e ao
    // loop de choose_action() em qlfd_choose_action()
    reg signed [DATA_WIDTH-1:0] best_q;
    reg        [WAY_BITS-1:0]   best_way_r;
    always @(*) begin
        best_q     = $signed(q_vals[0 +: DATA_WIDTH]);
        best_way_r = {WAY_BITS{1'b0}};
        for (w = 1; w < WAYS; w = w + 1) begin
            if ($signed(q_vals[w*DATA_WIDTH +: DATA_WIDTH]) > best_q) begin
                best_q     = $signed(q_vals[w*DATA_WIDTH +: DATA_WIDTH]);
                best_way_r = w[WAY_BITS-1:0];
            end
        end
    end

    // --- reward: +REWARD_HIT ou REWARD_MISS conforme is_hit ---
    wire signed [DATA_WIDTH-1:0] reward = is_hit ? REWARD_HIT_S : REWARD_MISS_S;

    // --- target = reward + (GAMMA * best_q) >> SCALE_BITS ---
    wire signed [2*DATA_WIDTH-1:0] prod_gamma  = GAMMA_S * best_q;
    wire signed [DATA_WIDTH-1:0]   gamma_term  = prod_gamma >>> SCALE_BITS;
    wire signed [DATA_WIDTH-1:0]   target      = reward + gamma_term;

    // --- diff = target - old_q ---
    wire signed [DATA_WIDTH-1:0] diff = target - old_q;

    // --- new_q = old_q + (ALPHA * diff) >> SCALE_BITS ---
    wire signed [2*DATA_WIDTH-1:0] prod_alpha  = ALPHA_S * diff;
    wire signed [DATA_WIDTH-1:0]   alpha_term  = prod_alpha >>> SCALE_BITS;
    wire signed [DATA_WIDTH-1:0]   new_q_s     = old_q + alpha_term;

    assign new_q    = new_q_s;
    assign best_way = best_way_r;

endmodule
