module FPGA_Wrapper(power, clk, rst);

	input power;
	input clk, rst;
	
	// Power switch
	// Signal power on when we have the on signal for at least 16 clock ticks
	reg [15:0] power_shift_register;
	reg power_officially_on; 
	
	LispMachine LispMachine(.power (power_officially_on),
									.clk(clk),
									.rst(rst));
					
	always@(posedge clk or negedge rst) begin
		if(!rst) begin
			power_shift_register <= 0;
			power_officially_on <= 0;
		end
		else begin
			if(!power_officially_on) begin
				power_shift_register <= {power_shift_register[14:0], power};
				
				power_officially_on = &power_shift_register;
			end
		end
	end

endmodule
