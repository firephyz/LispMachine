`include "ALU.vh"

module ALU(in_0, in_1, result, opcode);

	input [`alu_data_width - 1:0] in_0, in_1;
	input [`alu_opcode_width - 1:0] opcode;
	
	output reg [`alu_data_width - 1:0] result;
	
	always@(*) begin
		case(opcode)
			`SYS_ADD:
				result = in_0 + in_1;
			`SYS_SUB:
				result = in_0 - in_1;
			`SYS_AND:
				result = in_0 & in_1;
			`SYS_OR:
				result = in_0 | in_1;
			`SYS_NOT:
				result = ~in_0;
			`SYS_LESS:
				if(in_0 < in_1)
					result = 1;
				else
					result = 0;
			`SYS_GREATER:
				if(in_0 > in_1)
					result = 1;
				else
					result = 0;
			`SYS_EQUAL:
				if(in_0 == in_1)
					result = 1;
				else
					result = 0;
			default:
				result = 0;
		endcase
	end

endmodule
