module dmpresent (
        output  reg [63:0]  odat, 
        input               reset,
        input       [63:0]  idat,   
        input       [79:0]  key,   
        input               load,  
        input               clk,     
        output  reg         done
);


//---------wires, registers----------
reg     [79:0]  kreg;               // key register
reg     [63:0]  dreg;               // data register
reg     [63:0]  idreg;
reg     [4:0]   round;              // round counter
reg             active; 

wire    [63:0]  dat1, dat2, dat3;   // intermediate data
wire    [79:0]  kdat1, kdat2;       // intermediate subkey data 
wire    [63:0]  odreg;



assign odreg = dat1 ^ idreg;


// key generation
assign kdat1        = {kreg[18:0], kreg[79:19]}; // rotate key 61 bits to the left
assign kdat2[14:0 ] = kdat1[14:0 ];
assign kdat2[19:15] = kdat1[19:15] ^ round;  // xor key[19:15] data and round counter
assign kdat2[75:20] = kdat1[75:20];

sbox sBoxKey ( .data_in(kdat1[79:76]), .data_out(kdat2[79:76]) );


// add round key
assign dat1 = dreg ^ kreg[79:16]; 


// sBoxLayer
genvar i;
generate
    for (i=0; i<64; i=i+4) begin: sbox_loop
       sbox sBox( .data_in(dat1[i+3:i]), .data_out(dat2[i+3:i]) );
    end
endgenerate



// pBoxLayer
pbox pBox ( .data_in(dat2), .data_out(dat3) );

always @(posedge clk)
begin
    if (~reset) begin
        if (load) begin
            idreg   <= idat;
            dreg    <= idat;
            kreg    <= key;
            round   <= 1;
            odat    <= 64'h0;
            active  <= 1'b1;
        end

        else if (active) begin   
            dreg    <= dat3;
            kreg    <= kdat2;
            round   <= round + 1;
        end

        if (round == 0) begin
            odat    <= odreg; // or input data  
            done    <= 1'b1;
            active  <= 1'b0;
        end 

        else begin
            done    <= 1'b0;
        end
    end

    else begin
        odat <= 64'h0;
        done <= 1'b0;
    end
end

endmodule
