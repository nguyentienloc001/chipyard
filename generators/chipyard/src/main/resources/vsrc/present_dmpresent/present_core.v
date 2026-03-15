module present_core (
        output  reg [63:0]  odat,   
        input               reset,
        input       [63:0]  idat,  
        input       [79:0]  key,    
        input               load,   
        input               clk,    
        input               control, //=0=>encrypt; =1=>decrypt
        output  reg         done
);

wire    [63:0]  odreg_en, odreg_de;
reg     [63:0]  idreg;
reg     [79:0]  kdreg;
reg             chip_enable_en, chip_enable_de;
reg             load_reg;
wire            already_en,already_de;


always @(posedge clk) begin
    if(reset) begin
        chip_enable_en  <= 1'b0;
        chip_enable_de  <= 1'b0;
        idreg           <= 64'h0;
        kdreg           <= 80'h0;
        load_reg        <= 1'b0;
        odat            <= 64'h0;
    end

    else begin
        if (load) begin 
            idreg       <= idat;
            kdreg       <= key;
            load_reg    <= load;
            odat        <= odat;

            if(~control) begin
                chip_enable_en <= 1'b1;
                chip_enable_de <= 1'b0;
            end


            else begin
                chip_enable_en <= 1'b0;
                chip_enable_de <= 1'b1;
            end

        end

        else begin
            load_reg <= 1'b0;
            idreg <= idreg;
            kdreg <= kdreg;
            chip_enable_en <= chip_enable_en;
            chip_enable_de <= chip_enable_de;
        end


        if(chip_enable_en)
            odat <= odreg_en;
        else if (chip_enable_de )
                odat <= odreg_de;
        else
            odat <= odat;


        if (already_de)
            chip_enable_de <= 1'b0;

        if (already_en)
            chip_enable_en <= 1'b0;

        done <= already_de | already_en;
    end 


    // done <= already_de | already_en;

end

present_encrypt dut_en (
    .clk(clk),
    .chip_enable(chip_enable_en),
    .load(load_reg),
    .idat(idreg),
    .key(kdreg),
    .odat(odreg_en),
    .done(already_en)
);

present_decrypt dut_de (
    .clk(clk),
    .chip_enable(chip_enable_de),
    .load(load_reg),
    .idat(idreg),
    .key(kdreg),
    .odat(odreg_de),
    .done(already_de)
);

endmodule
