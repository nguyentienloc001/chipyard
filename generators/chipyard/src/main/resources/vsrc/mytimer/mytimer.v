/*
 * 		Khai Minh Ma - 2025
 */

module mytimer(
    input wire clk,           // Clock signal
    input wire reset,         // Reset signal

    input wire clear,         // Clear counter register
    input wire start,         // Start timer
    input wire pause,         // Pause timer
    output reg [31:0] count   // Counted cycles
);

    // State variable to track timer running state
    reg running;

    // Counter logic
    always @(posedge clk) begin

        if (reset == 1'b1) begin
            running <= 1'b0;
        end 

        else if (start == 1'b1) begin
            running <= 1'b1;   
        end 

        else if (pause == 1'b1) begin
            running <= 1'b0;   
        end

        else begin
        	running <= running;
        end

    end



    always @(posedge clk) begin

        if (reset == 1'b1) begin
            count <= 32'b0;
        end 

    	else if (clear == 1'b1) begin
    		count <= 32'b0;
    	end

        else if (running == 1'b1) begin
            count <= count + 1;
        end

        else begin
        	count <= count;
        end
    end

endmodule
