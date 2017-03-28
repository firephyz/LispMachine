`include "ALU.vh"
`include "memory_unit.vh"

module LispMachine(clk, rst);

	input clk, rst;

	// ALU interface
	reg [`alu_data_width - 1:0] alu_in_0, alu_in_1;
	reg [`alu_opcode_width - 1:0] alu_opcode;
	wire [`alu_data_width - 1:0] alu_result;

	ALU alu(.in_0(alu_in_0),
			  .in_1(alu_in_1),
			  .result(alu_result),
			  .opcode(alu_opcode));
	
	// Memory unit interface
	reg mem_car, mem_cdr, mem_cons;
	reg [`memory_data_width - 1:0] mem_data_in;
	wire [`memory_data_width - 1:0] mem_data_out;
	wire mem_ready;
	
	memory_unit mem(.get_car (mem_car),
					    .get_cdr (mem_cdr),
						 .get_cons (mem_cons),
					    .data_in (mem_data_in),
					    .data_out (mem_data_out),
					    .is_ready (mem_ready),
					    .clk (clk),
					    .rst (rst));
						 
	reg [3:0] counter;
	
	always@(posedge clk or negedge rst) begin
		if (!rst) begin
			counter <= 0;
			
			mem_car <= 0;
			mem_data_in <= 0;
		end
		else if (counter == 10) begin
			counter <= 11;
		
			mem_car <= 1;
			mem_data_in <= 24'h002405;
		end
		else if (counter < 10) begin
			counter <= counter + 1;
		
			mem_car <= 0;
			mem_data_in <= 24'h000000;
		end
		else if (counter == 11) begin
			mem_car <= 0;
			counter <= 12;
		end
		else begin
			if(!mem_ready) begin
				mem_car <= 0;
				mem_data_in <= 24'h000000;
			end
			else begin
				mem_car <= 1;
				mem_data_in <= mem_data_out;
			end
		end
	end

endmodule
