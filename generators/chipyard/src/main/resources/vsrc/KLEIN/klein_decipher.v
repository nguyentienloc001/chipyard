/*
	This module is used for klein decipher
	Huy Hoang Trinh - 2023
*/

module	klein_decipher (
	input	wire			iclk,
	input	wire			ireset,
	input	wire			istart,
	input	wire	[00:63]	iblock,
	input	wire	[00:63]	ikey,

	output	wire			oready,
	output	wire	[00:63]	oblock
);

reg				ready_reg;
reg 			ready_new;
reg 			ready_we;

reg 	[00:63] result_reg;
reg 			result_we;

reg		[00:03]	round; 
reg     [00:63] state, kstate;

wire    [00:63] nstate, nkstate;
wire    [00:63] ssum, ssubs, srot, smix;
wire    [00:63] krot, kfei;

// update output
assign oready = ready_reg;
assign oblock = result_reg;

// control
always @(posedge iclk) begin
	if (ireset) begin
		// reset
		round			<= 4'd11;
		result_reg		<= 64'd0;
		state 			<= 64'd0;
		kstate 			<= 64'd0;
		ready_reg		<= 1'b0;
	end
	else begin
		if(istart) begin
			round  		<=  4'd11;
			result_reg	<=  64'd0;
			state  		<=  iblock;
			kstate  	<=  ikey;
			ready_reg 	<= 	1'b0;
		end
		else begin
			if(round < 4'd12)
				round  		<=  round - 4'd1;
			state  		<=  nstate;
			kstate  	<=  nkstate;
			if(result_we)
				result_reg 	<= nstate ^ nkstate;
			if((round == 0))
				round		<= 4'd12;
			if(ready_we)
				ready_reg	<= ready_new;
		end
	end
end

// AddKeys
//
assign ssum = state ^ kstate;

// MixNibbles
//
klein_mixcolumn mix1  (.idata(ssum[00:31]), .iinv(1'b1), .odata(smix[00:31]));
klein_mixcolumn mix2  (.idata(ssum[32:63]), .iinv(1'b1), .odata(smix[32:63]));

// RotateNibble
//
assign srot = {smix[48:63], smix[00:47]};

//Sboxes
//
klein_sbox  s1    (.a(srot[00:03]), .y(nstate[00:03]));
klein_sbox  s2    (.a(srot[04:07]), .y(nstate[04:07]));
klein_sbox  s3    (.a(srot[08:11]), .y(nstate[08:11]));
klein_sbox  s4    (.a(srot[12:15]), .y(nstate[12:15]));
klein_sbox  s5    (.a(srot[16:19]), .y(nstate[16:19]));
klein_sbox  s6    (.a(srot[20:23]), .y(nstate[20:23]));
klein_sbox  s7    (.a(srot[24:27]), .y(nstate[24:27]));
klein_sbox  s8    (.a(srot[28:31]), .y(nstate[28:31]));
klein_sbox  s9    (.a(srot[32:35]), .y(nstate[32:35]));
klein_sbox  s10   (.a(srot[36:39]), .y(nstate[36:39]));
klein_sbox  s11   (.a(srot[40:43]), .y(nstate[40:43]));
klein_sbox  s12   (.a(srot[44:47]), .y(nstate[44:47]));
klein_sbox  s13   (.a(srot[48:51]), .y(nstate[48:51]));
klein_sbox  s14   (.a(srot[52:55]), .y(nstate[52:55]));
klein_sbox  s15   (.a(srot[56:59]), .y(nstate[56:59]));
klein_sbox  s16   (.a(srot[60:63]), .y(nstate[60:63]));

// stage 1
assign  kfei[00:15]  =  kstate[00:15];
assign  kfei[16:23]  =  kstate[16:23] ^ {4'd0, round+1'b1};
assign  kfei[24:39]  =  kstate[24:39];
klein_sbox  sk0   (.a(kstate[40:43]), .y(kfei[40:43]));
klein_sbox  sk1   (.a(kstate[44:47]), .y(kfei[44:47]));
klein_sbox  sk2   (.a(kstate[48:51]), .y(kfei[48:51]));
klein_sbox  sk3   (.a(kstate[52:55]), .y(kfei[52:55]));
assign  kfei[56:63]  =  kstate[56:63];

// stage 2
assign  krot[32:63]  =  kfei[00:31];
assign  krot[00:31]  =  kfei[00:31] ^ kfei[32:63];

// stage 3
assign  nkstate = {krot[24:31], krot[0:23], krot[56:63], krot[32:55]};


always @* begin
	result_we	= 1'b0;
	ready_new	= 1'b0;
	ready_we 	= 1'b0;

	if(round == 0) begin
		result_we	= 1'b1;
		ready_new	= 1'b1;
		ready_we 	= 1'b1;	
	end
end

endmodule
