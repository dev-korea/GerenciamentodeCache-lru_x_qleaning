// Memória da Q-table: q_table[set][rtag][way].
// Equivalente ao campo q_table de QLearningFixed / QLearningFixedDyn em C.
//
// Leitura síncrona (1 ciclo de latência): rd_en → rd_data válido quando rd_valid=1.
// A leitura retorna todas as WAYS Q-values de um (set, rtag) de uma vez,
// permitindo que o ql_update compute best_next e new_q sem acesso adicional.
//
// Escrita síncrona: atualiza uma única célula (set, rtag, way) por ciclo.
//
// Inicialização: initial block zera a memória (simulação + BRAM init no FPGA).
// Para síntese em FPGA, a ferramenta infere BRAM inicializada a zero por padrão.

module q_table #(
    parameter N_SETS       = 64,    // número de conjuntos da cache
    parameter REDUCED_TAGS = 16,    // REDUCED_TAGS = tag[3:0]
    parameter WAYS         = 2,     // número de vias (ações)
    parameter DATA_WIDTH   = 32     // largura de cada Q-value (bits)
)(
    input                              clk,
    input                              rst,

    // --- Porta de leitura (latência = 1 ciclo) ---
    input                              rd_en,
    input  [$clog2(N_SETS)-1:0]        rd_set,
    input  [$clog2(REDUCED_TAGS)-1:0]  rd_rtag,
    output [WAYS*DATA_WIDTH-1:0]       rd_data,   // {q[WAYS-1],...,q[1],q[0]}
    output                             rd_valid,  // pulso 1 ciclo após rd_en

    // --- Porta de escrita ---
    input                              wr_en,
    input  [$clog2(N_SETS)-1:0]        wr_set,
    input  [$clog2(REDUCED_TAGS)-1:0]  wr_rtag,
    input  [$clog2(WAYS)-1:0]          wr_way,
    input  [DATA_WIDTH-1:0]            wr_data
);
    localparam WAY_BITS = $clog2(WAYS);

    // Armazenamento principal
    reg signed [DATA_WIDTH-1:0] mem [0:N_SETS-1][0:REDUCED_TAGS-1][0:WAYS-1];

    // Todos os Q-values = 0 no início (equivale a init_qlearning_fixed)
    integer i, j, k;
    initial begin
        for (i = 0; i < N_SETS; i = i + 1)
            for (j = 0; j < REDUCED_TAGS; j = j + 1)
                for (k = 0; k < WAYS; k = k + 1)
                    mem[i][j][k] = 0;
    end

    // --- Escrita de uma célula ---
    always @(posedge clk) begin
        if (wr_en)
            mem[wr_set][wr_rtag][wr_way] <= $signed(wr_data);
    end

    // --- Leitura registrada de todas as vias ---
    reg [WAYS*DATA_WIDTH-1:0] rd_data_r;
    reg                        rd_valid_r;

    always @(posedge clk) begin
        if (rst) begin
            rd_data_r  <= {WAYS*DATA_WIDTH{1'b0}};
            rd_valid_r <= 1'b0;
        end else begin
            rd_valid_r <= rd_en;
            if (rd_en) begin
                for (k = 0; k < WAYS; k = k + 1)
                    rd_data_r[k*DATA_WIDTH +: DATA_WIDTH] <= mem[rd_set][rd_rtag][k];
            end
        end
    end

    assign rd_data  = rd_data_r;
    assign rd_valid = rd_valid_r;

endmodule
