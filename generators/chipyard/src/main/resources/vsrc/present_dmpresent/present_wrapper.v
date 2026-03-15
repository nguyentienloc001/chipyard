module present_wrapper(
    input   wire        clk,
    input   wire        iReset,
    input   wire        iChipselect,    
    input   wire        iWriteRead,
    input   wire [3:0]  iAddress,  
    input   wire [31:0] idat,   
    output  reg  [31:0] odat   
);


wire    [63:0]  odreg;
wire            done;

reg             reset;
reg     [63:0]  data;
reg     [79:0]  key;
reg             load;
reg             control;


always@(posedge clk)
begin
    if(iReset) begin
        reset   <= iReset;
        data    <= 64'b0;
        key     <= 80'b0;
        load    <= 1'b0;
        control <= 1'b0;
        odat    <= 32'b0;
    end

    else begin
        if(iChipselect) begin
            reset <= 1'b0;
        
            if(iWriteRead) begin        // iWriteRead = 1 <-> write
                odat <= 32'b0;
                case(iAddress[3:0])
                    4'd0:       load        <= idat[0];     // load/start
                    4'd1:       key[79:48]  <= idat;        // Key (80-bit)
                    4'd2:       key[47:16]  <= idat;
                    4'd3:       key[15:0]   <= idat[15:0];
                    4'd4:       data[63:32] <= idat;        // Data (64-bit)
                    4'd5:       data[31:0]  <= idat;
                    4'd8:       control     <= idat[0];     // control encrypt or decrypt
                    default:    odat        <= 32'b0;       // try to fix linter warning by giving default output
                endcase
            end

            else begin                  // iWriteRead = 0 <-> read
                load <= 1'b0;
                case(iAddress[3:0])
                    4'd6:       odat <= odreg[63:32];
                    4'd7:       odat <= odreg[31:0];
                    4'd8:       odat <= {31'b0, done};
                    default:    odat <= 32'b0; 
                endcase 
            end 


        end

        else begin
            load  <= 1'b0;
            reset <= iReset;
            odat  <= odat;
        end

    end
end

present_core dut(   
    .odat(odreg),
    .reset(reset),
    .idat(data),
    .key(key),
    .load(load),
    .clk(clk),
    .control(control),
    .done(done)
);


endmodule
