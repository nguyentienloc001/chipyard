/*
	This module is used for MixColumn and InvMixColumn
  Huy Hoang Trinh - 2023
*/

module klein_mixcolumn(
	input	   wire	[31:0]   idata,
	input	   wire          iinv,
	output	 wire	[31:0]   odata
);

function [7 : 0] gm2(input [7 : 0] op);
    begin
      gm2 = {op[6 : 0], 1'b0} ^ (8'h1b & {8{op[7]}});
    end
endfunction // gm2

function [7 : 0] gm4(input [7 : 0] op);
    begin
      gm4 = gm2(gm2(op));
    end
endfunction // gm4

wire	[7:0] w0, w1, w2, w3;	// data in
wire	[7:0] a0, a1, a2, a3;	// mix data
wire	[7:0] b0, b1, b2, b3;	// inv mix data

assign w0 = idata[31:24];
assign w1 = idata[23:16];
assign w2 = idata[15:08];
assign w3 = idata[07:00];

assign a0 = gm2(w0 ^ w1) ^ (w2 ^ w3) ^ w1;
assign a1 = gm2(w1 ^ w2) ^ (w3 ^ w0) ^ w2;
assign a2 = gm2(w2 ^ w3) ^ (w0 ^ w1) ^ w3;
assign a3 = gm2(w3 ^ w0) ^ (w1 ^ w2) ^ w0;

assign b0 = gm4(a0 ^ a2) ^ a0;
assign b1 = gm4(a1 ^ a3) ^ a1;
assign b2 = gm4(a0 ^ a2) ^ a2;
assign b3 = gm4(a1 ^ a3) ^ a3;

assign odata = (iinv) ? {b0, b1, b2, b3} : {a0, a1, a2, a3};

endmodule
