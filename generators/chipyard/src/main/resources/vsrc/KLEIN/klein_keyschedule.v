/*
	This module is generated the key round
	Huy Hoang Trinh - 2023
*/

module klein_keyschedule(
	input	wire			iclk,
	input	wire			ireset,
	input	wire			istart,
	input	wire	[00:63]	ikey,

	output 	wire	[00:63]	okey,
	output	wire			oready
);

reg				ready_reg;
reg 			ready_en;

reg		[00:63]	key_reg;
reg 			key_we;

reg		[0:3]	round;
reg		[00:63]	kstate;
wire	[00:63]	nkstate;
wire    [00:63] krot, kfei;

// update output
assign okey = key_reg;
assign oready = ready_reg;

// control
always @(posedge iclk) begin
	if(ireset) begin
		key_reg		<=	64'd0;
		round		<=	4'd0;
		ready_en	<=	1'b0;
		kstate 		<=	64'd0;
	end
	else begin
		if(istart) begin
			key_reg		<=	64'd0;
			round		<=	4'd0;
			ready_en	<=	1'b1;
			kstate 		<= 	ikey;		
		end
		else begin
			round 	<= 	round + 1;
			kstate 	<= 	nkstate;
			if(key_we)
				key_reg <= nkstate;

			if((round == 4'd11)) begin
				round <= 4'd0;
				if(ready_en)
					ready_en	<=	1'b0;
			end
		end		
	end
	
end

assign  krot  =  {kstate[8:31], kstate[0:7], kstate[40:63], kstate[32:39]};

// Feistel
//
assign  kfei[00:31]  =  krot[32:63];
assign  kfei[32:63]  =  krot[00:31] ^ krot[32:63];

// Next keys
//
assign  nkstate[00:15]  =  kfei[00:15];
assign  nkstate[16:23]  =  kfei[16:23] ^ { 4'd0 , round+1 };
assign  nkstate[24:39]  =  kfei[24:39];
klein_sbox  sk0  (.a(kfei[40:43]), .y(nkstate[40:43]));
klein_sbox  sk1  (.a(kfei[44:47]), .y(nkstate[44:47]));
klein_sbox  sk2  (.a(kfei[48:51]), .y(nkstate[48:51]));
klein_sbox  sk3  (.a(kfei[52:55]), .y(nkstate[52:55]));
assign  nkstate[56:63]  =  kfei[56:63];

always @* begin
	key_we		=	1'b0;
	ready_reg	=	1'b0;

	if((round == 4'd11) & ready_en) begin
		key_we		=	1'b1;
		ready_reg	=	1'b1;
	end
end

endmodule
