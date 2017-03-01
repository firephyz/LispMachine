`include "ALU.vh"

module ALU(in_0, in_1, result, opcode);

	input [`alu_data_width - 1:0] in_0, in_1;
	input [`alu_opcode_width - 1:0] opcode;
	
	output [`alu_data_width - 1:0] result;
	
	assign result = in_0 & in_1;

endmodule
