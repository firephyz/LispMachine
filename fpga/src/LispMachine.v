`include "ALU.vh"

module LispMachine(opcode, result);

	input wire [`alu_opcode_width - 1:0] opcode;
	output wire [`alu_data_width - 1:0] result;

	reg [`alu_data_width - 1:0] alu_in_0, alu_in_1;
	reg [`alu_opcode_width - 1:0] alu_opcode;
	wire [`alu_data_width - 1:0] alu_result;

	ALU alu(.in_0(alu_in_0),
			  .in_1(alu_in_1),
			  .result(alu_result),
			  .opcode(alu_opcode));
			  
	assign result = alu_result;
	
	always@(*) begin
		alu_opcode = opcode;
		alu_in_0 = 5;
		alu_in_1 = 7;
	end

endmodule
