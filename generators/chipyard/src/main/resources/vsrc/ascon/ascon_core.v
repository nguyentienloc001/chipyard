`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 10/11/2023 09:59:00 PM
// Design Name: 
// Module Name: ascon_core
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


module ascon_core
#(//Memory parameters
    parameter depth = 1024,
    parameter aw    = $clog2(depth),  
    parameter CCW = 32,
    parameter CCSW = 32,

  // Operation types
    parameter [4:0] OP_DO_ENC = 5'h0,
    parameter [4:0] OP_DO_DEC = 5'h1,
    parameter [4:0] OP_DO_HASH = 5'h2,
    parameter [4:0] OP_LD_KEY = 5'h3,
    parameter [4:0] OP_LD_NONCE = 5'h4,
    parameter [4:0] OP_LD_MS = 5'h5,
    parameter [4:0] OP_LD_PT = 5'h6,
    parameter [4:0] OP_LD_CT = 5'h7,
    parameter [4:0] OP_LD_TAG = 5'h8

    )(
    input wire 		   i_wb_clk,
    input wire 		   i_wb_rst,
    input wire [31:0]  i_wb_dat,
    output reg          o_rd_en,
    output reg          o_wr_en,
    output reg          tag_valid,
    output reg          o_done,
    input wire 		    i_start,
    input wire 		    in_fifo_empty,
    output reg [31:0]   o_wr_data,
    input wire [5:0]    fifo_in_counter,
    input wire          input_finish
    );
    
    reg [23:0] tb_word_cnt;
    reg [3:0] op;
    reg decryption;
    reg hash;
    reg [3:0] rcon;
    reg [2:0] byte_count;
    
    reg [63:0] x0_in;
    reg [63:0] x1_in;
    reg [63:0] x2_in;
    reg [63:0] x3_in;
    reg [63:0] x4_in;
   
    wire [63:0] x0_out;
    wire [63:0] x1_out;
    wire [63:0] x2_out;
    wire [63:0] x3_out;
    wire [63:0] x4_out;
       
    reg  [127:0] ascon_key;
    reg  [3:0] fsm_state;
  
    //managing reading ascon key
    // Write key when both valid and ready are high
    //shift 32 bits each cycle, 128 in total
    always @(posedge i_wb_clk) begin
        if (i_wb_rst) begin
            ascon_key <= 128'b0;
        end else if (op == OP_LD_KEY & tb_word_cnt > 1)
                ascon_key <= {ascon_key[95:0], i_wb_dat};
           else ascon_key <= ascon_key;
    end
    
    always@(posedge i_wb_clk) begin
        if(i_wb_rst) begin
            o_rd_en <= 1'b0;
            fsm_state <= 4'b0000;
            tb_word_cnt <= 24'b0;
            decryption <= 1'b0;
            hash <= 1'b0;
            op <= 4'b0;
            byte_count <= 3'b0;
        end
        else begin
        case(fsm_state)
        //idle state, wait for start signal
        4'b0000: begin
            o_rd_en <= 1'b0;
            decryption <= 1'b0;
            hash <= 1'b0;
            op <= 4'b0;
            tb_word_cnt <= 24'b0;
            byte_count <= 3'b0;
            if(i_start) begin
                fsm_state <= 4'b0001;
                //o_rd_en <= 1'b1;
            end
            else begin
                fsm_state <= 4'b0000;
            end
        end
        //buffer state, the memory need 2 cycles to respond
        4'b0001:begin
            if(!in_fifo_empty) begin
                fsm_state <= 4'b0010;
                o_rd_en <= 1'b1;
            end
        end
        //opcode for key
        4'b0010:begin
            tb_word_cnt <= (i_wb_dat[23:0] + 3) / 4 + 1;
            op <= i_wb_dat[31:28];
            fsm_state <= 4'b0011;
        end
        //load key or nonce state
        4'b0011: begin
            if(tb_word_cnt > 1) begin 
                fsm_state <= 4'b0011;
                tb_word_cnt <= tb_word_cnt - 1;
            end
            else begin
                tb_word_cnt <= (i_wb_dat[23:0] + 3) / 4 + 1;
                op <= i_wb_dat[31:28];
                if(op == OP_LD_KEY) begin
                    fsm_state <= 4'b0100;
                    o_rd_en <= 1'b1;
                end
                else if(op == OP_LD_NONCE) begin
                    fsm_state <= 4'b0101;
                    o_rd_en <= 1'b0;
                end
                else if(op == OP_LD_MS) begin
                    if(fifo_in_counter < 1) fsm_state <= 4'd3;
                    else begin
                        fsm_state <= 4'd5;
                        o_rd_en <= 1'b1;
                    end
                end
                
            end
        end
        //loa
        4'b0100: begin
            if(tb_word_cnt < 2) begin
                tb_word_cnt <= (i_wb_dat[23:0] + 3) / 4 + 1;
                op <= i_wb_dat[31:28];
                decryption <= op[0];
                hash <= op[1];
                o_rd_en <= 1'b0;
                if(op == 2) fsm_state <= 4'd3;
            end 
            else begin
                tb_word_cnt <= tb_word_cnt;
                op <= op;
                if(op == OP_LD_TAG) fsm_state <= 4'd0;
                else if(hash) begin
                    if(fifo_in_counter < 2) begin
                        fsm_state <= 4'd4;
                        o_rd_en <= 1'b0;
                    end
                    else begin 
                        o_rd_en <= 1'b1;
                        fsm_state <= 4'd5;
                    end
                end
                else begin 
                    if(fifo_in_counter < 7) begin 
                        fsm_state <= 4'd4; 
                        o_rd_en <= 1'b0; 
                    end
                    else begin 
                        fsm_state <= 4'd3;
                        o_rd_en <= 1'b1;
                    end
                end
            end
        end
        4'b0101: begin
            o_rd_en <= 1'b0;
            if(rcon > 1) begin
                fsm_state <= 4'b0101;
            end
            else begin
                if(fifo_in_counter < 2) fsm_state <= 4'd5;
                else fsm_state <= 4'd6;
            end
        end
        4'b0110: begin
            if(op == OP_LD_PT || op == OP_LD_CT) fsm_state <= 4'd10;
            else begin
                fsm_state <= 4'd7;
                tb_word_cnt <= tb_word_cnt - 1;
                o_rd_en <= 1'b1;
            end
        end
        4'b0111: begin
            fsm_state <= 4'b1000;
            tb_word_cnt <= tb_word_cnt - 1;
            //o_rd_en <= 1'b1;
        end
        4'b1000: begin
            fsm_state <= 9;
            o_rd_en <= 1'b0;
        end
        4'd9: begin
            if(rcon > 1) begin
                fsm_state <= 4'b1001;
            end
            else if(tb_word_cnt == 1) begin
                if(hash) begin 
                    fsm_state <= 10;
                    tb_word_cnt <= 9;
                    o_rd_en <= 1'b0;
                end
                else if(fifo_in_counter < 4) 
                     fsm_state <= 9;
                else begin
                    fsm_state <= 10;
                    tb_word_cnt <= (i_wb_dat[23:0] + 3) / 4 + 1;
                    o_rd_en <= 1'b1;
                    op <= i_wb_dat[31:28];
                end
            end 
            else begin
                //if(hash & fifo_in_counter < 2) begin
                //    fsm_state <= 4'd7;
                //    o_rd_en <= 1'b1;
                //    tb_word_cnt <= tb_word_cnt - 1;
                //end
                if((fifo_in_counter < 4 & !hash) | (hash & fifo_in_counter < 2)) fsm_state <= 4'd9;
                else begin
                    fsm_state <= 4'b0111;
                    o_rd_en <= 1'b1;
                    tb_word_cnt <= tb_word_cnt - 1;
                end
            end
        end
        4'd10: begin
            fsm_state <= 4'd11;
            tb_word_cnt <= tb_word_cnt - 1;
            if(!hash) o_rd_en <= 1'b1;
        end
        4'd11: begin
            fsm_state <= 4'd12;
            if(!hash) o_rd_en <= 1'b1;
        end
        4'd12: begin
            fsm_state <= 4'd13;
            tb_word_cnt <= tb_word_cnt - 1;
            o_rd_en <= 1'b0;
            
        end
        4'd13: begin
            if(rcon > 1) begin
                fsm_state <= 4'd13;
            end
            else if(tb_word_cnt == 1) begin
                if(input_finish) fsm_state <= 4'd0;
                else if(fifo_in_counter < 6) fsm_state <= 4'd13;
                else fsm_state <= 4'd14;
            end 
            else begin
                if(hash) begin
                    fsm_state <= 4'd11;
                    tb_word_cnt <= tb_word_cnt - 1;
                end
                else if(fifo_in_counter < 3) fsm_state <= 4'd13;
                else begin 
                    fsm_state <= 4'd11;
                    tb_word_cnt <= tb_word_cnt - 1;
                    o_rd_en <= 1'b1;
                end
            end
        end
        4'd14: begin
            if(!hash) fsm_state <= 4'd15;
            else fsm_state <= 4'd0;
            byte_count <= 6;
            if(decryption) o_rd_en <= 1'b1;
            else tb_word_cnt <= tb_word_cnt - 1;
        end
        4'd15: begin            
            if(byte_count > 1) begin
                fsm_state <= 4'd15;
                byte_count <= byte_count - 1;
                if(byte_count == 2 ) o_rd_en <= 1'b1;
            end
            else begin
                //if(fifo_in_counter < 7) fsm_state <= 4'd15;
                //else begin
                    fsm_state <= 4'd4;
                    o_rd_en <= 1'b1;
                    tb_word_cnt <= (i_wb_dat[23:0] + 3) / 4 + 1;
                    op <= i_wb_dat[31:28];
                //end
            end
        end
        endcase
        end
    end
    
    always @(posedge i_wb_clk) begin
        if (i_wb_rst) begin
            x0_in <= 64'b0;
            x1_in <= 64'b0;
            x2_in <= 64'b0;
            x3_in <= 64'b0;
            x4_in <= 64'b0;
        end else if (op == OP_LD_NONCE & tb_word_cnt > 1) begin
                x0_in <= 64'h80400c0600000000;
                x1_in <= ascon_key[127:64];
                x2_in <= ascon_key[63:0];
                x3_in <= {x3_in[31:0], x4_in[63:32]};
                x4_in <= {x4_in[31:0], i_wb_dat};
            end
            else if (op == OP_LD_MS & fsm_state == 3) begin
                x0_in <= 64'h00400c0000000100;
                x1_in <= 64'b0;
                x2_in <= 64'b0;
                x3_in <= 64'b0;
                x4_in <= 64'b0;
            end
        else if((fsm_state == 5 | fsm_state == 9 | fsm_state == 13) & rcon > 0) begin
                x0_in <= x0_out;
                x1_in <= x1_out;
                x2_in <= x2_out;
                x3_in <= x3_out;
                x4_in <= x4_out;
            end
        else if((fsm_state == 6 | fsm_state == 14) & !hash) begin
                x3_in <= x3_in ^ ascon_key[127:64];
                x4_in <= x4_in ^ ascon_key[63:0]; 
        end
        else if(fsm_state == 7 | (fsm_state == 11 & !decryption & !hash)) begin
                x0_in[63:32] <= x0_in[63:32] ^ i_wb_dat;
        end
        else if(fsm_state == 11 & decryption & !hash) begin
                x0_in[63:32] <= i_wb_dat;
        end
        else if((fsm_state == 8) | (fsm_state == 12 & tb_word_cnt > 2 & !decryption & !hash)) begin
                x0_in[31:0] <= x0_in[31:0] ^ i_wb_dat;
        end
        else if(fsm_state == 12 & tb_word_cnt > 2 & decryption & !hash) begin
                x0_in[31:0] <= i_wb_dat;
        end
        else if(fsm_state == 10 & !hash) begin
                x4_in[0] <= x4_in[0] ^ 1'b1;
        end
        else if(fsm_state == 12 & tb_word_cnt == 2 & !decryption & !hash) begin
                x1_in <= x1_in ^ ascon_key[127:64];
                x2_in <= x2_in ^ ascon_key[63:0]; 
                x0_in[31:0] <= x0_in[31:0] ^ i_wb_dat;
        end
        else if(fsm_state == 12 & tb_word_cnt == 2 & decryption & !hash) begin
                x1_in <= x1_in ^ ascon_key[127:64];
                x2_in <= x2_in ^ ascon_key[63:0]; 
                x0_in[31:0] <= i_wb_dat;
        end
    end
    
    always @(posedge i_wb_clk) begin
        if(i_wb_rst) rcon <= 4'b0;
        else if( (fsm_state == 3 & op == OP_LD_NONCE & tb_word_cnt == 1) | (fsm_state == 12 & tb_word_cnt == 2) | (fsm_state == 3 & op == OP_LD_MS & tb_word_cnt == 1) | ((fsm_state == 8 | fsm_state == 12) & hash)) rcon <= 4'hC;
        else if((fsm_state == 8 | fsm_state == 12) & !hash) rcon <= 4'h6;
        else if((fsm_state == 5 | fsm_state == 9 | fsm_state == 13) & rcon > 4'b0)  rcon <= rcon - 4'b1;
        else rcon <= 4'b0;
    end
    
    always @(posedge i_wb_clk) begin
        if(i_wb_rst) o_done <= 1'b0;
        else if(fsm_state == 4 & op == OP_LD_TAG)  o_done <= 1'b1;
    end
    
    always @(posedge i_wb_clk) begin
        if (i_wb_rst) begin
            o_wr_data <= 32'b0;
            o_wr_en <= 1'b0;
        end
        else if(fsm_state == 11) begin
            if(!hash) o_wr_data <= x0_in[63:32] ^ i_wb_dat;
            else o_wr_data <= x0_in[63:32];
            o_wr_en <= 1'b1;
        end
        else if(fsm_state == 12) begin
            if(!hash) o_wr_data <= x0_in[31:0] ^ i_wb_dat;
            else o_wr_data <= x0_in[31:0];
            o_wr_en <= 1'b1;
        end
        else if(fsm_state == 15 & byte_count == 6) begin
            o_wr_data <= x3_in[63:32];
            o_wr_en <= 1'b1;
        end
        else if(fsm_state == 15 & byte_count == 5) begin
            o_wr_data <= x3_in[31:0];
            o_wr_en <= 1'b1;
            if(o_wr_data == i_wb_dat) tag_valid <= 1'b1;
        end
        else if(fsm_state == 15 & byte_count == 4) begin
            o_wr_data <= x4_in[63:32];
            o_wr_en <= 1'b1;
            if(o_wr_data == i_wb_dat) tag_valid <= 1'b1;
        end
        else if(fsm_state == 15 & byte_count == 3) begin
            o_wr_data <= x4_in[31:0];
            o_wr_en <= 1'b1;
            if(o_wr_data == i_wb_dat) tag_valid <= 1'b1;
        end
        else if(fsm_state == 15 & byte_count == 2) begin
                o_wr_en <= 1'b0;
                if(o_wr_data == i_wb_dat) tag_valid <= 1'b1;
        end
        else begin
            o_wr_data <= 32'b0;
            o_wr_en <= 1'b0;
            tag_valid <= 1'b0;
        end
    end 
    asconp asconp_inst(
                        .rcon(rcon),
                        .x0_in(x0_in),
                        .x1_in(x1_in),
                        .x2_in(x2_in),
                        .x3_in(x3_in),
                        .x4_in(x4_in),
                        .x0_out(x0_out),
                        .x1_out(x1_out),
                        .x2_out(x2_out),
                        .x3_out(x3_out),
                        .x4_out(x4_out)
    );
    

endmodule
