/*
	This module is a klein core (include cipher and decipher)
	Huy Hoang Tring - 2023
*/

module klein_core(
	input	wire			iclk,
	input	wire			ireset,
	input	wire			iencdec, // 1-enci, 0-deci
	input	wire			iinit,
	input	wire			inext,
	input	wire	[0:63]	ikey,
	input	wire	[0:63]	iblock,

	output 	wire			oready,
	output	wire			oresult_valid,
	output	wire	[0:63]	oblock
	);

// parameter
//
parameter	CTRL_IDLE	= 2'h0;
parameter	CTRL_INIT	= 2'h1;
parameter	CTRL_NEXT	= 2'h2;

// stage reg
//
reg	[1:0]	klein_ctrl_reg;
reg [1:0]	klein_ctrl_new;
reg [1:0]	klein_ctrl_we;

reg 		ready_reg;
reg 		ready_new;
reg 		ready_we;

reg 		result_valid_reg;
reg 		result_valid_new;
reg 		result_valid_we;

// wire
//
wire	[00:63]	key_wire;
wire 			key_ready; // in init stage

reg 			enc_next;
wire 	[00:63] enc_oblock;
wire 			enc_ready;

reg 			dec_next;
wire 	[00:63] dec_oblock;
wire 			dec_ready;

reg 	[00:63] mux_oblock;
reg 			mux_ready;

reg 	[00:63] result_oblock; // for synchronization

// instantiation
//
klein_cipher cipher_block(
	.iclk(iclk),
	.ireset(ireset),
	.istart(enc_next),
	.iblock(iblock),
	.ikey(ikey),
	.oready(enc_ready),
	.oblock(enc_oblock)
	);


klein_decipher	decipher_block(
	.iclk(iclk),
	.ireset(ireset),
	.istart(dec_next),
	.iblock(iblock),
	.ikey(key_wire),
	.oready(dec_ready),
	.oblock(dec_oblock)
	);

klein_keyschedule keyschedule(
	.iclk(iclk),
	.ireset(ireset),
	.istart(iinit),
	.ikey(ikey),
	.okey(key_wire),
	.oready(key_ready)
	);

// connect ouput
//
assign oblock 			= result_oblock;
assign oready			= ready_reg;
assign oresult_valid	= result_valid_reg;

// control
//
always @(posedge iclk) begin
	if (ireset) begin
		// reset
		ready_reg			<= 1'b0;
		result_valid_reg	<= 1'b0;
		klein_ctrl_reg		<= CTRL_IDLE;
	end
	else begin
		result_oblock 			<= mux_oblock;

		if(ready_we)
			ready_reg			<= ready_new;

		if(result_valid_we)
			result_valid_reg	<= result_valid_new;

		if(klein_ctrl_we)
			klein_ctrl_reg		<= klein_ctrl_new;
	end
end

// encipher and decipher mux
//
always @* begin
	enc_next = 1'b0;
	dec_next = 1'b0;

	if(iencdec) begin
		enc_next 	= inext;
		mux_oblock 	= enc_oblock;
		mux_ready	= enc_ready;
	end
	else begin
		dec_next	= inext;
		mux_oblock	= dec_oblock;
		mux_ready	= dec_ready;
	end
end

// FSM
//
always @* begin
	ready_new			= 1'b0;
	ready_we			= 1'b0;
	result_valid_new	= 1'b0;
	result_valid_we 	= 1'b0;
	klein_ctrl_new		= CTRL_IDLE;
	klein_ctrl_we		= 1'b0;

	case (klein_ctrl_reg)
		CTRL_IDLE: begin
			if(iinit) begin
				ready_new			= 1'b0;
				ready_we			= 1'b1;
				result_valid_new	= 1'b0;
				result_valid_we 	= 1'b1;
				klein_ctrl_new		= CTRL_INIT;
				klein_ctrl_we		= 1'b1;
			end
			else if(inext) begin
				ready_new			= 1'b0;
				ready_we			= 1'b1;
				result_valid_new	= 1'b0;
				result_valid_we 	= 1'b1;
				klein_ctrl_new		= CTRL_NEXT;
				klein_ctrl_we		= 1'b1;
			end
		end

		CTRL_INIT: begin
			if(key_ready) begin
				ready_new			= 1'b1;
				ready_we			= 1'b1;
				klein_ctrl_new		= CTRL_IDLE;
				klein_ctrl_we		= 1'b1;
			end
		end

		CTRL_NEXT: begin
			if(mux_ready) begin
				ready_new			= 1'b1;
				ready_we			= 1'b1;
				result_valid_new	= 1'b1;
				result_valid_we 	= 1'b1;
				klein_ctrl_new		= CTRL_IDLE;
				klein_ctrl_we		= 1'b1;
			end
		end

		default:
			begin
			
			end
	endcase
end

endmodule
