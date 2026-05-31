// Array de cache associativa por conjuntos.
// Equivalente às funções em cache.c / cache_dyn.c:
//   init_cache, find_hit, find_empty_line, fill_line
//
// Leitura (hit + empty): combinacional, result disponível no mesmo ciclo.
// Escrita (fill): síncrona, commit no próximo clk.

module cache_array #(
    parameter N_SETS    = 64,
    parameter WAYS      = 2,
    parameter TAG_WIDTH = 21    // ADDR_WIDTH - BLOCK_BITS - SET_BITS
)(
    input                        clk,
    input                        rst,

    // --- Porta de leitura (lookup) ---
    input  [$clog2(N_SETS)-1:0]  set_addr,   // conjunto a consultar
    input  [TAG_WIDTH-1:0]       rd_tag,      // tag da requisição

    output                       hit,          // 1 = acerto
    output [$clog2(WAYS)-1:0]    hit_way,      // via do acerto

    output                       has_empty,    // 1 = existe via livre
    output [$clog2(WAYS)-1:0]    empty_way,    // primeira via livre

    // --- Porta de escrita (fill) ---
    input                        wr_en,
    input  [$clog2(N_SETS)-1:0]  wr_set,
    input  [$clog2(WAYS)-1:0]    wr_way,
    input  [TAG_WIDTH-1:0]       wr_tag
);
    localparam WAY_BITS = $clog2(WAYS);

    // Armazenamento: valid e tag por via/conjunto
    reg [TAG_WIDTH-1:0] tags  [0:N_SETS-1][0:WAYS-1];
    reg                 valid [0:N_SETS-1][0:WAYS-1];

    integer i, j;

    // Reset síncrono e escrita (fill)
    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < N_SETS; i = i + 1)
                for (j = 0; j < WAYS; j = j + 1) begin
                    valid[i][j] <= 1'b0;
                    tags [i][j] <= {TAG_WIDTH{1'b0}};
                end
        end else if (wr_en) begin
            tags [wr_set][wr_way] <= wr_tag;
            valid[wr_set][wr_way] <= 1'b1;
        end
    end

    // Detecção de hit: verifica todas as vias em paralelo
    reg                 hit_r;
    reg [WAY_BITS-1:0]  hit_way_r;

    always @(*) begin
        hit_r     = 1'b0;
        hit_way_r = {WAY_BITS{1'b0}};
        for (i = 0; i < WAYS; i = i + 1) begin
            if (!hit_r && valid[set_addr][i] && tags[set_addr][i] == rd_tag) begin
                hit_r     = 1'b1;
                hit_way_r = i;   // trunca para WAY_BITS
            end
        end
    end

    // Detecção de via livre: primeira via com valid=0
    reg                 has_empty_r;
    reg [WAY_BITS-1:0]  empty_way_r;

    always @(*) begin
        has_empty_r = 1'b0;
        empty_way_r = {WAY_BITS{1'b0}};
        for (i = 0; i < WAYS; i = i + 1) begin
            if (!has_empty_r && !valid[set_addr][i]) begin
                has_empty_r = 1'b1;
                empty_way_r = i;
            end
        end
    end

    assign hit       = hit_r;
    assign hit_way   = hit_way_r;
    assign has_empty = has_empty_r;
    assign empty_way = empty_way_r;

endmodule
