/*
	This module is a klein wrapper
	Huy Hoang Trinh - 2023
*/

module klein(
	input	wire			iclk,
	input	wire			ireset,
	input	wire			ics,
	input	wire			iwe,
	input	wire	[07:0]	iaddress,
	input	wire	[31:0]	iwrite_data,

	output	wire	[31:0]	oread_data
	);

// parameter
//
parameter ADDR_CTRL			= 8'h00;
parameter CTRL_INIT_BIT  	= 0;
parameter CTRL_NEXT_BIT		= 1;

parameter ADDR_CONF			= 8'h01;
parameter CONF_ENCDEC_BIT	= 0;

parameter ADDR_STATUS		= 8'h02;
parameter STATUS_READY_BIT	= 0;
parameter STATUS_VALID_BIT	= 1;

parameter ADDR_KEY0			= 8'h10;
parameter ADDR_KEY1			= 8'h11;

parameter ADDR_BLOCK0		= 8'h20;
parameter ADDR_BLOCK1		= 8'h21;

parameter ADDR_RESULT0		= 8'h30;
parameter ADDR_RESULT1		= 8'h31;

// stage reg
//
reg				init_reg;
reg				init_new;

reg				next_reg;
reg				next_new;

reg 			encdec_reg;
reg  			config_we;

reg  	[0:31]	block_reg 	[0:1];
reg 			block_we;

reg 	[0:31]	key_reg 	[0:1];
reg 			key_we;

reg 	[0:63] 	result_reg;
reg 			valid_reg;
reg 			ready_reg;

// wire
//
reg 	[0:31] 	tmp_read_data;

wire			core_iencdec;
wire			core_iinit;
wire			core_inext;
wire	[0:63]	core_ikey;
wire	[0:63]	core_iblock;
wire			core_oready;
wire			core_ovalid;
wire 	[0:63]	core_oblock;

// conncect
//
assign oread_data 	= tmp_read_data;

assign core_ikey 	= {key_reg[0], key_reg[1]};
assign core_iblock 	= {block_reg[0], block_reg[1]};
assign core_iinit 	= init_reg;
assign core_inext 	= next_reg;
assign core_iencdec = encdec_reg;

// instantiation
//
klein_core core(
	.iclk(iclk),
	.ireset(ireset),
	.iencdec(core_iencdec),
	.iinit(core_iinit),
	.inext(core_inext),
	.ikey(core_ikey),
	.iblock(core_iblock),
	.oready(core_oready),
	.oresult_valid(core_ovalid),
	.oblock(core_oblock)
	);

// control
//
always @(posedge iclk) begin
	if (ireset) begin
		// reset
		block_reg[0]	<=	32'd0;
		block_reg[1]	<=	32'd0;
		key_reg[0]		<=	32'd0;
		key_reg[1]		<=	32'd0;
		init_reg		<=	1'b0;
		next_reg		<=	1'b0;
		encdec_reg		<=	1'b0;

		result_reg		<=	64'd0;
		valid_reg		<=	1'b0;
		ready_reg		<=	1'b0;
	end
	else begin
		ready_reg		<=	core_oready;
		valid_reg		<=	core_ovalid;
		result_reg		<=	core_oblock;
		init_reg		<=	init_new;
		next_reg		<=	next_new;

		if(config_we) 
			encdec_reg	<=	iwrite_data[CONF_ENCDEC_BIT];

		if(key_we)
			key_reg[iaddress[0]]	<= iwrite_data;

		if(block_we)
			block_reg[iaddress[0]]	<= iwrite_data;
	end
end

// combination
//
always @* begin
	init_new		=	1'b0;
	next_new		=	1'b0;
	config_we		=	1'b0;
	key_we			=	1'b0;
	block_we		=	1'b0;
	tmp_read_data	=	32'd0;

	if(ics) begin
		if(iwe) begin
			if(iaddress == ADDR_CONF)
				config_we 	= 1'b1;

			if(iaddress == ADDR_CTRL) begin
				init_new 	= iwrite_data[CTRL_INIT_BIT];
				next_new	= iwrite_data[CTRL_NEXT_BIT];
			end

			if((iaddress >= ADDR_KEY0) && (iaddress <= ADDR_KEY1))
                key_we = 1'b1;

            if((iaddress >= ADDR_BLOCK0) && (iaddress <= ADDR_BLOCK1))
                block_we = 1'b1;
		end
		else begin
			if(iaddress == ADDR_STATUS)
                tmp_read_data = {30'h0, valid_reg, ready_reg};

			if((iaddress >= ADDR_RESULT0) && (iaddress <= ADDR_RESULT1)) begin
				tmp_read_data = result_reg[(iaddress - ADDR_RESULT0) * 32 +: 32];
			end
		end
	end
end
endmodule
