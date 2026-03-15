/**
 * 	Module "ROM - Read-Only-Memory" for testing
 **/

module rom_8x32(
	input 				clk,
	input 		[2:0] 	addr,
	output reg	[31:0] 	dout
);


always@(posedge clk)
begin
	case(addr)
		3'h0: dout <= 32'hDEADBEEF;
		3'h1: dout <= 32'h12345678;
		3'h2: dout <= 32'h87654321;
		3'h3: dout <= 32'hF000000F;
		3'h4: dout <= 32'h28042000;
		3'h5: dout <= 32'h03022000;
		3'h6: dout <= 32'h06082001;
		3'h7: dout <= 32'hFFFFFFFF;
		default:
			begin
			end
	endcase


end // EOF "always"

endmodule

