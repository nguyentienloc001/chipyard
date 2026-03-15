
module dmpresent_wrapper(
    input           clk,
    input           iReset,
    input           iChipselect,    
    input           iWriteRead,    
    input   [2:0]   iAddress,
    input   [31:0]  idat, 
    output reg [31:0] odat
);

/*Address:
    0000:   load
    00xx:   iKey: 0x1[79:48]; 0x2[47:16]; 0x3[15:0]
    010x:   iDat: 0x4;0x5
    011x:   oData:0x6,0x7
*/

reg             reset;
reg     [63:0]  data;
reg     [79:0]  key;
reg             load;

wire            done;
wire    [63:0]  odreg;


always@(posedge clk) begin
    if(iReset) begin
        odat    <= 32'b0;
        reset   <= iReset;
        data    <= 64'b0;
        key     <= 80'b0;
        load    <= 1'b0;
    end

    else begin
        if(iChipselect) begin
            reset <= 1'b0;

            //input
            if(iWriteRead) begin
                odat <= 32'b0;
                case(iAddress[2:0])
                    //load
                    3'd0:   load <= idat[0];
                    
                    //key
                    3'd1:   key[79:48] <= idat;
                    3'd2:   key[47:16] <= idat;
                    3'd3:   key[15:0]  <= idat[15:0];
                    
                    //iData
                    3'd4:   data[63:32] <= idat;
                    3'd5:   data[31:0]  <= idat;
                    default: odat <= 32'b0;
                endcase
            end

            else begin
                load <= 1'b0;
                case(iAddress[2:0])
                    3'd0:       odat <= done;
                    3'd6:       odat <= odreg[63:32];
                    3'd7:       odat <= odreg[31:0];
                    default:    odat <= 32'b0;
                endcase 
            end 

        end // EOF "iChipselect"


        else begin
            reset   <= iReset;
            odat    <= 32'b0;
            load    <= 1'b0;
        end


    end
end

dmpresent dut(
    .clk(clk),
    .reset(reset),

    .load(load),
    .idat(data),
    .key(key),

    .odat(odreg), 
    .done(done)
);

endmodule
