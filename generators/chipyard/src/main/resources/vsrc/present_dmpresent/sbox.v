module sbox (
   input  wire  [3:0] data_in,
   output reg   [3:0] data_out
);

always @(data_in)
    case (data_in)
        4'h0: data_out = 4'hC;
        4'h1: data_out = 4'h5;
        4'h2: data_out = 4'h6;
        4'h3: data_out = 4'hB;
        4'h4: data_out = 4'h9;
        4'h5: data_out = 4'h0;
        4'h6: data_out = 4'hA;
        4'h7: data_out = 4'hD;
        4'h8: data_out = 4'h3;
        4'h9: data_out = 4'hE;
        4'hA: data_out = 4'hF;
        4'hB: data_out = 4'h8;
        4'hC: data_out = 4'h4;
        4'hD: data_out = 4'h7;
        4'hE: data_out = 4'h1;
        4'hF: data_out = 4'h2;
    endcase

endmodule
