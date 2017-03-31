`include "ALU.vh"
`include "memory_unit.vh"

module LispMachine(power, clk, rst);

	input power, clk, rst;

	// ALU interface
	reg [`alu_data_width - 1:0] alu_in_0, alu_in_1;
	reg [`alu_opcode_width - 1:0] alu_opcode;
	wire [`alu_data_width - 1:0] alu_result;

	ALU alu(.in_0(alu_in_0),
			  .in_1(alu_in_1),
			  .result(alu_result),
			  .opcode(alu_opcode));
	
	// Eval unit to memory unit interface
	wire [1:0] mem_func;
	wire mem_execute;
	wire [`memory_addr_width - 1:0] addr0, addr1;
	wire [3:0] type_info;
	wire [`memory_addr_width - 1:0] addr_out;
	wire [`memory_data_width - 1:0] data_out;
	wire mem_ready;
	
	memory_unit mem(.func (mem_func),
						 .execute (mem_execute),
						 .addr0 (addr0),
						 .addr1 (addr1),
						 .type_info (type_info),
						 .addr_out (addr_out),
						 .data_out (data_out),
					    .is_ready (mem_ready),
						 .power (power),
					    .clk (clk),
					    .rst (rst));
						 
	eval_unit eval(.mem_func (mem_func),
						.mem_execute (mem_execute),
						.mem_addr0 (addr0),
						.mem_addr1 (addr1),
						.mem_type_info (type_info),
						.mem_addr (addr_out),
						.mem_data (data_out),
						.mem_ready (mem_ready),
						.power (power),
						.clk (clk),
						.rst (rst));

endmodule
