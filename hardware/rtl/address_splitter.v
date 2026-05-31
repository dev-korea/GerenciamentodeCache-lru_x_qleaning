// Decompõe endereço em set_idx e tag para cache associativa por conjuntos.
// Equivalente a split_address() / split_address_dyn() em cache.c.
//
// L1 (32B block, 64 sets): BLOCK_BITS=5, SET_BITS=6
//   set_idx = address[10:5]   tag = address[31:11]
//
// L2 (64B block, 64 sets): BLOCK_BITS=6, SET_BITS=6
//   set_idx = address[11:6]   tag = address[31:12]

module address_splitter #(
    parameter ADDR_WIDTH = 32,
    parameter BLOCK_BITS = 5,   // log2(block_size)
    parameter SET_BITS   = 6    // log2(n_sets)
)(
    input  [ADDR_WIDTH-1:0]                     address,
    output [SET_BITS-1:0]                       set_idx,
    output [ADDR_WIDTH-BLOCK_BITS-SET_BITS-1:0] tag
);
    assign set_idx = address[BLOCK_BITS + SET_BITS - 1 : BLOCK_BITS];
    assign tag     = address[ADDR_WIDTH - 1 : BLOCK_BITS + SET_BITS];

endmodule
