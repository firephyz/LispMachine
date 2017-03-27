`include "ALU.vh"
`include "memory_unit.vh"

module LispMachine(clk, rst);

	input clk, rst;

	reg [`alu_data_width - 1:0] alu_in_0, alu_in_1;
	reg [`alu_opcode_width - 1:0] alu_opcode;
	wire [`alu_data_width - 1:0] alu_result;

	ALU alu(.in_0(alu_in_0),
			  .in_1(alu_in_1),
			  .result(alu_result),
			  .opcode(alu_opcode));
	
	reg mem_car, mem_cdr;
	reg [`memory_data_width - 1:0] mem_data_in, mem_data_out;
	wire mem_ready;
	
	memory_unit mem(.get_car (mem_car),
					    .get_cdr (mem_cdr),
					    .data_in (mem_data_in),
					    .data_out (mem_data_out),
					    .is_ready (mem_ready),
					    .clk (clk),
					    .rst (rst));

endmodule
