/***************************************************************************************
 * 
 * SHA3-512 wrapper 
 * 
 * Author: Khai Minh Ma
 * 
 ***************************************************************************************/

module keccak_wrapper(
	input	iClk,		
	input	iReset,

	input	iChipSelect,		
	input	iWrite,
	input 	iRead,			

	input [7:0]  	iAddress,
	input [31:0]  	iWriteData,		// Directly connected to keccak

	output reg [31:0]  	oReadData
);


/*****************************************************************************
 *           Internal Wires, Registers and Paramemters Declarations          *
 *****************************************************************************/

// in 			- 32-bit: 	input data MSWord -> LSWord
// in_ready		- 1-bit:	data-in indicator
// is_last		- 1-bit:	data-in is last word indicator
// byte_num		- 2-bit: 	size of the "is_last" data (in bytes) to cut -> "padder1.v"
// buffer_full  - 1-bit: 	buffer is full, begin to hash?
// out 			- 512-bit:	hash output (digest)
// out_ready 	- 1-bit:


reg [31:0]		rKeccak_input;
reg				rKeccak_in_ready;
reg 			rKeccak_is_last;
reg [1:0]		rKeccak_byte_num;

wire [511:0] 	wKeccak_out;
wire 			wKeccak_out_ready;	
wire 			wKeccak_buffer_full;

wire 			wKeccak_in_ready;
wire 			wKeccak_is_last;
wire [1:0]		wKeccak_byte_num;


reg 			rReset_internal;

reg 			rAllow_in_ready;

/*****************************************************************************
 *                              Internal Modules                             *
 *****************************************************************************/
keccak DUT(
	.clk 		(iClk), 
	.reset 		(rReset_internal), 

	.in  		(iWriteData), 				// iWriteData or rKeccak_input?
	.in_ready 	(rKeccak_in_ready), 
	.is_last  	(rKeccak_is_last),
	.byte_num 	(rKeccak_byte_num), 

	.buffer_full(wKeccak_buffer_full), 
	.out  		(wKeccak_out), 
	.out_ready  (wKeccak_out_ready)
);



/*****************************************************************************
 *                            Combinational Logic                            *
 *****************************************************************************/

// Temp
// assign oReadData = wKeccak_out[511:(512-32)];




/*****************************************************************************
 *                             Sequential Logic                              *
 *****************************************************************************/
always @(posedge iClk or posedge iReset) 
begin
	if (iReset) 
	begin
		rKeccak_input 		<= 32'h0; // ??????
		rReset_internal 	<= iReset;
		rKeccak_in_ready	<= 1'b0;
		rKeccak_is_last 	<= 1'b0;
		rKeccak_byte_num	<= 2'b00;
	end


	else if (iChipSelect) 
	begin
		rReset_internal 	<= iReset;
		rKeccak_in_ready	<= 1'b0;
		rKeccak_is_last 	<= 1'b0;

		if(iWrite)
		begin
			case(iAddress)
				8'h00:	rReset_internal		<= 1'h1;
				8'h01:
					begin 
						rKeccak_in_ready 	<= 1'b1 & rAllow_in_ready;
						rKeccak_is_last 	<= 1'b0;
					end

				8'h02:	rKeccak_byte_num 	<= iWriteData[1:0];
				8'h03:
					begin
						rKeccak_in_ready 	<= 1'b1 & rAllow_in_ready;
						rKeccak_is_last 	<= 1'b1;
						//rKeccak_byte_num is configured at ADDR 0x02
					end

				default:
					begin
					end

			endcase
		end


		else if(iRead)
		begin
			case(iAddress)
				8'h0f:	oReadData <= {30'h0, wKeccak_buffer_full, wKeccak_out_ready};
				8'h10:	oReadData <= wKeccak_out[511:480];
				8'h11:	oReadData <= wKeccak_out[479:448];
				8'h12:	oReadData <= wKeccak_out[447:416];
				8'h13:	oReadData <= wKeccak_out[415:384];
				8'h14:	oReadData <= wKeccak_out[383:352];
				8'h15:	oReadData <= wKeccak_out[351:320];
				8'h16:	oReadData <= wKeccak_out[319:288];
				8'h17:	oReadData <= wKeccak_out[287:256];
				8'h18:	oReadData <= wKeccak_out[255:224];
				8'h19:	oReadData <= wKeccak_out[223:192];
				8'h1a:	oReadData <= wKeccak_out[191:160];
				8'h1b:	oReadData <= wKeccak_out[159:128];
				8'h1c:	oReadData <= wKeccak_out[127:96];
				8'h1d:	oReadData <= wKeccak_out[95:64];
				8'h1e:	oReadData <= wKeccak_out[63:32];
				8'h1f:	oReadData <= wKeccak_out[31:0];
				default:
					begin
					end
			endcase
		end

	end


	else
	begin
		rReset_internal 	<= iReset; 
		rKeccak_in_ready	<= 1'b0;
		rKeccak_is_last 	<= 1'b0;
	end
end


always @(posedge iClk or posedge iReset)
begin
	if (iReset) 
		rAllow_in_ready <= 1'b1; 	// Always allow?


	else if(iChipSelect)
	begin
	 	if(iWrite)
	 	begin
	 		if(iAddress == 8'h01 || iAddress == 8'h03)
				rAllow_in_ready <= 1'b0;

			else
				rAllow_in_ready <= 1'b1;
	 	end


	 	else if(iRead) 
		begin
			rAllow_in_ready <= 1'b1;
		end
	end // EOF iChipSelect

	else 
	begin
		rAllow_in_ready <= 1'b1;
	end


end


endmodule
