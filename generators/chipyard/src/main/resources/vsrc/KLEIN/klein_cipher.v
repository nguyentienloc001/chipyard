/*
    This module is a klein cipher
    Huy Hoang Trinh - 2023
*/

module  klein_cipher (
  input   wire          iclk,
  input   wire          ireset,
  input   wire          istart,
  input   wire  [00:63] iblock,
  input   wire  [00:63] ikey,

  output  wire          oready,
  output  wire  [00:63] oblock
);

reg             ready_reg;
reg             ready_new;
reg             ready_we;

reg     [00:63] result_reg;
reg             result_we;

reg     [00:03] round; 
reg     [00:63] state, kstate;

wire    [00:63] nstate, nkstate;
wire    [00:63] ssum, ssubs, srot;
wire    [00:63] krot, kfei;

// update output
//
assign oready = ready_reg;
assign oblock = result_reg;

// control
//
always @(posedge iclk) begin
    if (ireset) begin
        // reset
        round       <= 4'd0;
        result_reg  <= 64'd0;
        state       <= 64'd0;
        kstate      <= 64'd0;
        ready_reg   <= 1'b0;
    end
    else begin
        if(istart) begin
            round       <=  4'd0;
            result_reg  <= 64'd0;
            state       <=  iblock;
            kstate      <=  ikey;
            ready_reg   <=  1'b0;
        end
        else begin
            if(round < 4'd12)
                round       <=  round + 4'd1;
            state     <=  nstate;
            kstate    <=  nkstate;
            if(result_we)
                result_reg  <= nstate ^ nkstate;
            if(round == 4'd11)
                round       <= 4'd12;
            if(ready_we)
                ready_reg   <= ready_new;
        end
    end
end

// AddKeys
//
assign ssum = state ^ kstate;

//Sboxes
//
klein_sbox  s1   (.a(ssum[00:03]), .y(ssubs[00:03]));
klein_sbox  s2   (.a(ssum[04:07]), .y(ssubs[04:07]));
klein_sbox  s3   (.a(ssum[08:11]), .y(ssubs[08:11]));
klein_sbox  s4   (.a(ssum[12:15]), .y(ssubs[12:15]));
klein_sbox  s5   (.a(ssum[16:19]), .y(ssubs[16:19]));
klein_sbox  s6   (.a(ssum[20:23]), .y(ssubs[20:23]));
klein_sbox  s7   (.a(ssum[24:27]), .y(ssubs[24:27]));
klein_sbox  s8   (.a(ssum[28:31]), .y(ssubs[28:31]));
klein_sbox  s9   (.a(ssum[32:35]), .y(ssubs[32:35]));
klein_sbox  s10  (.a(ssum[36:39]), .y(ssubs[36:39]));
klein_sbox  s11  (.a(ssum[40:43]), .y(ssubs[40:43]));
klein_sbox  s12  (.a(ssum[44:47]), .y(ssubs[44:47]));
klein_sbox  s13  (.a(ssum[48:51]), .y(ssubs[48:51]));
klein_sbox  s14  (.a(ssum[52:55]), .y(ssubs[52:55]));
klein_sbox  s15  (.a(ssum[56:59]), .y(ssubs[56:59]));
klein_sbox  s16  (.a(ssum[60:63]), .y(ssubs[60:63]));

// RotateNibbles
//
assign  srot  =  { ssubs[16:63] , ssubs[0:15] } ;

// MixNibbles
//
klein_mixcolumn mix1(.idata(srot[00:31]), .iinv(1'b0), .odata(nstate[00:31]));
klein_mixcolumn mix2(.idata(srot[32:63]), .iinv(1'b0), .odata(nstate[32:63]));

// Rotate keys
//
assign  krot  =  { kstate[8:31] , kstate[0:7] , kstate[40:63] , kstate[32:39] } ;

// Feistel
//
assign  kfei[00:31]  =  krot[32:63] ;
assign  kfei[32:63]  =  krot[00:31]  ^  krot[32:63] ;

// Next keys
//
assign  nkstate[00:15]  =  kfei[00:15];
assign  nkstate[16:23]  =  kfei[16:23]  ^  { 4'd0 , round+1 };
assign  nkstate[24:39]  =  kfei[24:39];
klein_sbox  sk0  (.a(kfei[40:43]), .y(nkstate[40:43]));
klein_sbox  sk1  (.a(kfei[44:47]), .y(nkstate[44:47]));
klein_sbox  sk2  (.a(kfei[48:51]), .y(nkstate[48:51]));
klein_sbox  sk3  (.a(kfei[52:55]), .y(nkstate[52:55]));
assign  nkstate[56:63]  =  kfei[56:63] ;


always @* begin
    result_we   = 1'b0;
    ready_new   = 1'b0;
    ready_we    = 1'b0;
  
    if(round == 4'd11) begin
        result_we   = 1'b1;
        ready_new   = 1'b1;
        ready_we    = 1'b1; 
    end
end

endmodule
