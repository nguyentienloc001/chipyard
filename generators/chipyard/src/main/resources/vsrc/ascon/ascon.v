`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/04/2023 04:00:13 PM
// Design Name: 
// Module Name: ascon
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module ascon
  #(//Memory parameters
    parameter depth = 1024,
    parameter aw    = $clog2(depth),  
    parameter CCW = 32,
    parameter CCSW = 32
    )
   (input wire 		   i_clk,
    input wire 		   i_rst,
    input wire [31:0]  i_data,
    input wire         i_write_data,
    input wire         i_start,
    output wire        in_fifo_full,
    output wire [31:0] out_fifo_data,
    output wire        out_fifo_empty,
    output wire        o_done,
    output wire        tag_valid,
    input wire         out_fifo_rd_en,
    input wire         input_finish
);
  wire [31:0] ascon_wr_data;
  wire [31:0] fifo2Ascon_data;
  wire [5:0] fifo_in_counter;


    fifo fifo_in(
    .clk(i_clk),
    .rst(i_rst),
    .buf_in(i_data),
    .buf_out(fifo2Ascon_data),
    .wr_en(i_write_data),
    .rd_en(rd_en),
    .buf_empty(in_fifo_empty),
    .buf_full(in_fifo_full),
    .fifo_counter(fifo_in_counter)
    );
    
    
    ascon_core ascon_inst(
    .i_wb_clk(i_clk),
    .i_wb_rst(i_rst),
    .i_wb_dat(fifo2Ascon_data),
    .o_rd_en(rd_en),
    .o_done(o_done),
    .tag_valid(tag_valid),
    .i_start(i_start),
    .in_fifo_empty(in_fifo_empty),
    .o_wr_en(o_write_data),
    .o_wr_data(ascon_wr_data),
    .fifo_in_counter(fifo_in_counter),
    .input_finish(input_finish)
    );
    
     fifo fifo_out(
    .clk(i_clk),
    .rst(i_rst),
    .buf_in(ascon_wr_data),
    .buf_out(out_fifo_data),
    .wr_en(o_write_data),
    .rd_en(out_fifo_rd_en),
    .buf_empty(out_fifo_empty),
    .buf_full(),
    .fifo_counter()
    );
    
    
     

endmodule
