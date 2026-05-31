`timescale 1ns/1ps

// Testbench LRU — cache L1 (64 sets, 2 vias, bloco 32B)
//
// Replica os três traces de run_lru() em simulator.c usando os módulos:
//   address_splitter, cache_array, lru_ctrl
//
// O testbench faz o papel do cache_fsm: avalia os sinais combinacionais
// e aciona wr_en/upd_en antes da borda de subida do clock.
//
// Resultados esperados (conforme Feedback-1.txt):
//   Trace 1 — Boa Localidade : 6 hits, 2 misses (75.00%)
//   Trace 2 — Conflito Forte :  0 hits, 9 misses  (0.00%)
//   Trace 3 — Misto          : 3 hits, 9 misses (25.00%)

module tb_lru;

    // =========================================================
    // Parâmetros — espelham config.h para L1
    // =========================================================
    localparam ADDR_WIDTH = 32;
    localparam BLOCK_BITS = 5;                              // log2(32 B)
    localparam SET_BITS   = 6;                              // log2(64 sets)
    localparam TAG_WIDTH  = ADDR_WIDTH - BLOCK_BITS - SET_BITS; // 21
    localparam N_SETS     = 64;
    localparam WAYS       = 2;
    localparam WAY_BITS   = 1;                              // log2(2)

    // =========================================================
    // Clock e reset
    // =========================================================
    reg clk, rst;
    initial clk = 1'b0;
    always #5 clk = ~clk;   // período 10 ns → 100 MHz

    // =========================================================
    // Sinais conectados à DUT
    // =========================================================

    // address_splitter
    reg  [ADDR_WIDTH-1:0] address;
    wire [SET_BITS-1:0]   set_idx;
    wire [TAG_WIDTH-1:0]  tag;

    // cache_array — porta de leitura (combinacional)
    wire                  hit;
    wire [WAY_BITS-1:0]   hit_way;
    wire                  has_empty;
    wire [WAY_BITS-1:0]   empty_way;

    // cache_array — porta de escrita (síncrona)
    reg                   wr_en;
    reg  [SET_BITS-1:0]   wr_set;
    reg  [WAY_BITS-1:0]   wr_way;
    reg  [TAG_WIDTH-1:0]  wr_tag;

    // lru_ctrl
    reg                   upd_en;
    reg  [SET_BITS-1:0]   upd_set;
    reg  [WAY_BITS-1:0]   upd_way;
    wire [WAY_BITS-1:0]   victim_way;

    // =========================================================
    // Instâncias dos módulos RTL
    // =========================================================

    address_splitter #(
        .ADDR_WIDTH (ADDR_WIDTH),
        .BLOCK_BITS (BLOCK_BITS),
        .SET_BITS   (SET_BITS)
    ) u_split (
        .address (address),
        .set_idx (set_idx),
        .tag     (tag)
    );

    cache_array #(
        .N_SETS    (N_SETS),
        .WAYS      (WAYS),
        .TAG_WIDTH (TAG_WIDTH)
    ) u_cache (
        .clk       (clk),
        .rst       (rst),
        .set_addr  (set_idx),
        .rd_tag    (tag),
        .hit       (hit),
        .hit_way   (hit_way),
        .has_empty (has_empty),
        .empty_way (empty_way),
        .wr_en     (wr_en),
        .wr_set    (wr_set),
        .wr_way    (wr_way),
        .wr_tag    (wr_tag)
    );

    lru_ctrl #(
        .N_SETS (N_SETS),
        .WAYS   (WAYS)
    ) u_lru (
        .clk        (clk),
        .rst        (rst),
        .upd_en     (upd_en),
        .upd_set    (upd_set),
        .upd_way    (upd_way),
        .rd_set     (set_idx),   // vítima sempre do conjunto atual
        .victim_way (victim_way)
    );

    // =========================================================
    // Variáveis de teste (compartilhadas pelas tasks)
    // =========================================================
    integer hits_count;
    integer misses_count;

    // Registradores locais para capturar sinais combinacionais
    reg [SET_BITS-1:0]  l_set;
    reg [TAG_WIDTH-1:0] l_tag;
    reg [WAY_BITS-1:0]  l_way;

    // =========================================================
    // Task: executa um único acesso à cache com política LRU.
    // Equivale a uma iteração do loop principal de run_lru().
    //
    // Fluxo de temporização:
    //   negedge → dirige endereço, lê saídas combinacionais,
    //             aciona enables
    //   posedge → cache_array e lru_ctrl registram as alterações
    // =========================================================
    task do_access;
        input [ADDR_WIDTH-1:0] addr;
        begin
            // Apresenta endereço na descida para estabilizar combinacional
            @(negedge clk);
            address = addr;
            #1; // propaga combinacional

            // Captura sinais estáveis
            l_set = set_idx;
            l_tag = tag;

            if (hit) begin
                // HIT: atualiza LRU, sem escrita na cache
                hits_count = hits_count + 1;
                l_way   = hit_way;
                wr_en   = 1'b0;
                upd_en  = 1'b1;  upd_set = l_set;  upd_way = l_way;
                $display("  [HIT ] addr=0x%04X  set=%0d  tag=%0d  via=%0d",
                         addr, l_set, l_tag, l_way);
            end else begin
                // MISS: decide se preenche via livre ou expulsa vítima LRU
                misses_count = misses_count + 1;
                if (has_empty) begin
                    l_way = empty_way;
                    $display("  [MISS] addr=0x%04X  set=%0d  tag=%0d  -> preenche via livre %0d",
                             addr, l_set, l_tag, l_way);
                end else begin
                    l_way = victim_way;
                    $display("  [MISS] addr=0x%04X  set=%0d  tag=%0d  -> expulsa vitima LRU via %0d",
                             addr, l_set, l_tag, l_way);
                end
                wr_en  = 1'b1;  wr_set = l_set;  wr_way = l_way;  wr_tag = l_tag;
                upd_en = 1'b1;  upd_set = l_set;  upd_way = l_way;
            end

            // Borda de subida: registradores atualizam
            @(posedge clk); #1;

            // Desativa enables após commit
            wr_en  = 1'b0;
            upd_en = 1'b0;
        end
    endtask

    // =========================================================
    // Task: exibe contagem final e verifica contra esperado
    // =========================================================
    task show_result;
        input integer exp_hits;
        input integer exp_misses;
        real  hit_rate;
        integer total;
        begin
            total    = hits_count + misses_count;
            hit_rate = (total > 0) ? (hits_count * 100.0 / total) : 0.0;
            $display("  ------------------------------------------");
            $display("  Hits: %0d | Misses: %0d | Hit Rate: %.2f%%",
                     hits_count, misses_count, hit_rate);
            if (hits_count === exp_hits && misses_count === exp_misses)
                $display("  RESULTADO: OK  (esperado %0d hits, %0d misses)",
                         exp_hits, exp_misses);
            else
                $display("  RESULTADO: ERRO  esperado=%0dH/%0dM  obtido=%0dH/%0dM",
                         exp_hits, exp_misses, hits_count, misses_count);
            $display("");
        end
    endtask

    // =========================================================
    // Task: reinicia a DUT entre traces
    // =========================================================
    task reset_dut;
        begin
            rst          = 1'b1;
            wr_en        = 1'b0;
            upd_en       = 1'b0;
            address      = {ADDR_WIDTH{1'b0}};
            hits_count   = 0;
            misses_count = 0;
            repeat (2) @(posedge clk);
            rst = 1'b0;
            @(posedge clk);
        end
    endtask

    // =========================================================
    // Sequência principal de testes
    // =========================================================
    initial begin
        $dumpfile("tb_lru.vcd");
        $dumpvars(0, tb_lru);

        reset_dut();

        // -------------------------------------------------------
        // TRACE 1: Boa Localidade
        // Endereços: {0x0000, 0x0800} × 4
        // set=0, tags 0 e 1 alternados — 2 vias bastam
        // Esperado: 6 hits, 2 misses (75.00%)
        // -------------------------------------------------------
        $display("==============================================");
        $display("TRACE 1: Boa Localidade");
        $display("==============================================");
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h0000);
        do_access(32'h0800);
        show_result(6, 2);

        // -------------------------------------------------------
        // TRACE 2: Conflito Forte
        // Endereços: {0x0000, 0x0800, 0x1000} × 3
        // set=0, 3 tags diferentes disputam 2 vias — thrashing
        // Esperado: 0 hits, 9 misses (0.00%)
        // -------------------------------------------------------
        reset_dut();
        $display("==============================================");
        $display("TRACE 2: Conflito Forte");
        $display("==============================================");
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h1000);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h1000);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h1000);
        show_result(0, 9);

        // -------------------------------------------------------
        // TRACE 3: Misto
        // Endereços: {0x0000,0x0800,0x0000,0x0800,0x1000,0x0000,
        //             0x0800,0x1000,0x0000,0x0800,0x0000,0x1000}
        // Mistura localidade com conflito
        // Esperado: 3 hits, 9 misses (25.00%)
        // -------------------------------------------------------
        reset_dut();
        $display("==============================================");
        $display("TRACE 3: Misto");
        $display("==============================================");
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h1000);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h1000);
        do_access(32'h0000);
        do_access(32'h0800);
        do_access(32'h0000);
        do_access(32'h1000);
        show_result(3, 9);

        $display("==============================================");
        $display("Simulacao concluida.");
        $display("==============================================");
        $finish;
    end

endmodule
