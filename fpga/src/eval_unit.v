
`include "memory_unit.vh"

module eval_unit(mem_ready, mem_func, mem_execute, mem_addr0, mem_addr1, mem_type_info, mem_addr, mem_data, power, clk, rst, mem_powered_up, sys_func, state, result, stack, calling_func, arg_push_count);

	input power, clk, rst;
	
	output reg mem_powered_up;
	reg set_stack;
	
	// Interface with the memory unit
	input mem_ready;
	output reg [1:0] mem_func;
	output reg mem_execute;
	output reg [`memory_addr_width - 1:0] mem_addr0, mem_addr1;
	output reg [3:0] mem_type_info;
	input [`memory_addr_width - 1:0] mem_addr;
	input [`memory_data_width - 1:0] mem_data;
	
	// Machine registers
	reg [`memory_addr_width - 1:0] regs [0:3];
	output reg [`memory_addr_width - 1:0] result;
	reg [`memory_data_width - 1:0] data_regs [0:3];
	
	output reg [`memory_addr_width - 1:0] stack;
	output reg [3:0] calling_func;
	output reg [2:0] arg_push_count;
	//reg [`memory_addr_width - 1:0] push_regs [0:3];
	
	// General counter for counting things.
	reg [3:0] counter;
	
	// Cell type encodings
	parameter CELL_GENERAL	= 4'h0,
				 CELL_OPCODE	= 4'h1,
				 CELL_SYMBOL	= 4'h2,
				 CELL_NUMBER	= 4'h3,
				 CELL_BOOL		= 4'h4,
				 CELL_RETURN	= 4'h5; // Used internally to identify return records on the system stack
				 
	// Cell symbol type encodings
	parameter OP_ADD			= 10'h0,
				 OP_SUB			= 10'h1,
				 OP_LESS			= 10'h2,
				 OP_EQUAL		= 10'h3,
				 OP_GREATER		= 10'h4,
				 OP_AND			= 10'h5,
				 OP_OR			= 10'h6,
				 OP_NOT			= 10'h7,
				 OP_CAR			= 10'h8,
				 OP_CDR			= 10'h9,
				 OP_CONS			= 10'hA,
				 OP_EQ			= 10'hB,
				 OP_ATOM			= 10'hC,
				 OP_IF			= 10'hD,
				 OP_LAMBDA		= 10'hE,
				 OP_QUOTE		= 10'hF,
				 OP_DEFINE		= 10'h10,
				 OP_BEGIN		= 10'h11;
				 
	// Calling function encodings
	parameter SYS_EVAL_0		= 4'h0,
				 SYS_EVAL_1		= 4'hF, // TODO Put in right order once we figure out the code associated with this
				 SYS_APPLY_0	= 4'h1,
				 SYS_APPLY_1	= 4'h2,
				 SYS_APPLY_2	= 4'h3,
				 SYS_EVLIS_0	= 4'h4,
				 SYS_EVLIS_1	= 4'h5,
				 SYS_EVIF		= 4'h6,
				 SYS_EVBEGIN	= 4'h7,
				 SYS_CONENV		= 4'h8,
				 SYS_LOOKUP		= 4'h9,
				 SYS_RETURN		= 4'hA,
				 SYS_EQ			= 4'hB,
				 SYS_REPL		= 4'hF;
	
	// System functions for use with sys_func
	parameter SYS_FUNC_EVAL		= 4'h0,
				 SYS_FUNC_APPLY	= 4'h1,
				 SYS_FUNC_EVLIS	= 4'h2,
				 SYS_FUNC_EVIF		= 4'h3,
				 SYS_FUNC_EVARTH	= 4'h4,
				 SYS_FUNC_CONENV	= 4'h5,
				 SYS_FUNC_LOOKUP	= 4'h6,
				 SYS_FUNC_RETURN	= 4'h7,
				 SYS_FUNC_PUSH		= 4'h8, // Don't need a pop because it's only
												  // called once. Effectively inlined it
				 SYS_FUNC_EQ		= 4'h9,
				 SYS_FUNC_DONE		= 4'hA;
				 
	// Eval states
	parameter SYS_EVAL_INIT 			= 5'h0,
				 SYS_EVAL_FETCH_0_WAIT	= 5'h1,
				 SYS_EVAL_FIRST_IF		= 5'h2,
				 SYS_EVAL_FETCH_1_WAIT	= 5'h3,
				 SYS_EVAL_SWITCH			= 5'h4,
				 SYS_EVAL_QUOTE_WAIT		= 5'h5,
				 SYS_EVAL_IF_FETCH_0		= 5'h6,
				 SYS_EVAL_IF_FETCH_1		= 5'h7,
				 SYS_EVAL_IF_FETCH_2		= 5'h8,
				 SYS_EVAL_PUSH_0			= 5'h9,
				 SYS_EVAL_PUSH_1			= 5'hA,
				 SYS_EVAL_EVLIS_SETUP	= 5'hB,
				 SYS_EVAL_EVLIS_CONT 	= 5'hC,
				 SYS_EVAL_EVLIS_CONT_WAIT = 5'hD;
				 
	// Apply states
	parameter SYS_APPLY_INIT				= 5'h00,
				 SYS_APPLY_GET_SECOND_ARG	= 5'h01,
				 SYS_APPLY_STORE_ARGS		= 5'h02,
				 SYS_APPLY_IS_ATOM			= 5'h03,
				 SYS_APPLY_OP_CAR_0			= 5'h04,
				 SYS_APPLY_OP_CDR_0			= 5'h05,
				 SYS_APPLY_OP_CONS_0			= 5'h06,
				 SYS_APPLY_OP_CONS_RETURN	= 5'h07,
				 SYS_APPLY_OP_EQ_CALL		= 5'h08,
				 SYS_APPLY_OP_ATOM			= 5'h09,
				 SYS_APPLY_COMPARE_0			= 5'h0A,
				 SYS_APPLY_COMPARE_1			= 5'h0B,
				 SYS_APPLY_COMPARE_EXECUTE	= 5'h0C,
				 SYS_APPLY_LOGIC_0			= 5'h0D,
				 SYS_APPLY_CALL_EVAL			= 5'h0E,
				 SYS_APPLY_CALL_APPLY		= 5'h0F,
				 SYS_APPLY_CALL_CONENV		= 5'h10,
				 SYS_APPLY_CALL_CONENV_1	= 5'h11,
				 SYS_APPLY_CALL_EVAL_1		= 5'h12,
				 SYS_APPLY_CALL_EVAL_2		= 5'h13,
				 SYS_APPLY_CALL_EVAL_3		= 5'h14,
				 SYS_APPLY_ARITH_0			= 5'h15,
				 SYS_APPLY_ARITH_1			= 5'h16,
				 SYS_APPLY_ARITH_WAIT		= 5'h17,
				 SYS_APPLY_ARITH_WAIT_1		= 5'h18;
	
	// Evlis states
	parameter SYS_EVLIS_INIT			= 5'h0,
				 SYS_EVLIS_CALL_EVAL_0 	= 5'h1,
				 SYS_EVLIS_CALL_EVAL_1	= 5'h2,
				 SYS_EVLIS_PUSH_TWO		= 5'h3,
				 SYS_EVLIS_CALL_EVLIS_0	= 5'h4,
				 SYS_EVLIS_CALL_EVLIS_1	= 5'h5,
				 SYS_EVLIS_GET_CONS		= 5'h6,
				 SYS_EVLIS_RETURN			= 5'h7;
	
	// Evif states
	parameter SYS_EVIF_INIT				= 5'h0,
				 SYS_EVIF_CALL_EVAL		= 5'h1,
				 SYS_EVIF_CHOOSE			= 5'h2;
	
	// Evarth states
	parameter SYS_EVARTH_INIT			= 5'h0,
				 SYS_EVARTH_GET_0			= 5'h1,
				 SYS_EVARTH_GET_1			= 5'h2,
				 SYS_EVARTH_GET_1_CAR	= 5'h3,
				 SYS_EVARTH_RECURSE		= 5'h4,
				 SYS_EVARTH_CONS_WAIT	= 5'h5;
	
	// Conenv states
	parameter SYS_CONENV_INIT				= 5'h0,
				 SYS_CONENV_GET_ARG			= 5'h1,
				 SYS_CONENV_GET_CONS			= 5'h2,
				 SYS_CONENV_PUSH_ARGS		= 5'h3,
				 SYS_CONENV_CALL_CONENV		= 5'h4,
				 SYS_CONENV_RETURN			= 5'h5,
				 SYS_CONENV_DONE				= 5'h6;
	
	// Lookup states
	parameter SYS_LOOKUP_INIT			= 5'h0,
				 SYS_LOOKUP_SETUP_REGS_0 = 5'h1,
				 SYS_LOOKUP_SETUP_REGS_1 = 5'h2,
				 SYS_LOOKUP_SETUP_REGS_2 = 5'h3,
				 SYS_LOOKUP_CHECK_EQ		= 5'h4;
	
	// Return states
	parameter SYS_RETURN_INIT					= 5'h0,
				 SYS_RETURN_RESTORE_CALLING	= 5'h1,
				 SYS_RETURN_POP_STACK			= 5'h2,
				 SYS_RETURN_DONE					= 5'h3;
				 
	// Push states
	parameter SYS_PUSH_INIT				= 5'h0,
				 SYS_PUSH_WAIT_CONS		= 5'h1,
				 SYS_PUSH_LAST_PART		= 5'h2,
				 SYS_PUSH_LAST_WAIT		= 5'h3;
				 
	// Eq states
	parameter SYS_EQ_INIT			= 5'h0,
				 SYS_EQ_FETCH_FIRST	= 5'h1,
				 SYS_EQ_FETCH_SECOND = 5'h2,
				 SYS_EQ_CHECK			= 5'h3;
	
	output reg [3:0] sys_func; // Records the current system function
	output reg [4:0] state; // Records the state inside the current system function
	
	always@(posedge clk or negedge rst) begin
		if(!rst) begin
			mem_powered_up <= 0;
			set_stack <= 0;
			
			sys_func <= SYS_FUNC_EVAL;
			state <= SYS_EVAL_INIT;
			
			regs[0] <= 1;
			regs[1] <= 0;
			regs[2] <= 0;
			regs[3] <= 0;
			
			//stack <= mem_addr;
			stack <= 0;
		end
		else if(!set_stack && mem_powered_up) begin
			set_stack <= 1;
			stack <= mem_addr;
		end
		else if(power && mem_powered_up && set_stack) begin
			case(sys_func)
				SYS_FUNC_EVAL: begin
					case (state)
						SYS_EVAL_INIT: begin
							mem_addr0 <= regs[0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_EVAL_FETCH_0_WAIT;
						end
						SYS_EVAL_FETCH_0_WAIT: begin
							if(mem_ready) begin
								state <= SYS_EVAL_FIRST_IF;
								data_regs[0] <= mem_data;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVAL_FIRST_IF: begin
							// If cell is atom
							if(data_regs[0][23:20] != 4'h0) begin
								// Dispatch according to the type of the atom
								if(data_regs[0][23:20] == CELL_SYMBOL) begin
									state <= SYS_LOOKUP_INIT;
									sys_func <= SYS_FUNC_LOOKUP;
								end
								else if (data_regs[0][23:20] == CELL_BOOL) begin
									result <= regs[0];
									
									state <= SYS_RETURN_INIT;
									sys_func <= SYS_FUNC_RETURN;
								end
								else if (data_regs[0][23:20] == CELL_NUMBER) begin
									result <= regs[0];
									
									state <= SYS_RETURN_INIT;
									sys_func <= SYS_FUNC_RETURN;
								end
								else begin
									result <= 10'h3FF; // Error
									
									state <= SYS_RETURN_INIT;
									sys_func <= SYS_FUNC_RETURN;
								end
							end
							// Is not an atom
							else begin
								// We need to fetch the car of this cell to do further processing
								mem_addr0 <= data_regs[0][19:10];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVAL_FETCH_1_WAIT;
							end
						end
						SYS_EVAL_FETCH_1_WAIT: begin
							if(mem_ready) begin
								state <= SYS_EVAL_SWITCH;
								data_regs[1] <= mem_data;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						/*
						At this point. Regs are:
						data_regs[0] = Cont(regs[0])
						data_regs[1] = Cont(car(regs[0]))
						*/
						SYS_EVAL_SWITCH: begin
							case(data_regs[1][7:0])
								OP_IF: begin
									regs[3] <= regs[1];
									
									mem_addr0 <= data_regs[0][9:0];
									mem_func <= `GET_CONTENTS;
									mem_execute <= 1;
									
									state <= SYS_EVAL_IF_FETCH_0;
								end
								OP_LAMBDA: begin
									result <= regs[0];
									
									state <= SYS_RETURN_INIT;
									sys_func <= SYS_FUNC_RETURN;
								end
								OP_QUOTE: begin
									mem_addr0 <= data_regs[0][9:0];
									mem_func <= `GET_CONTENTS;
									mem_execute <= 1;
									
									state <= SYS_EVAL_QUOTE_WAIT;
								end
								OP_DEFINE:; // TODO
								OP_BEGIN:; // TODO
								default: begin
									calling_func <= SYS_EVAL_0;
									arg_push_count <= 2;
									
									sys_func = SYS_FUNC_PUSH;
									state <= SYS_PUSH_INIT;
								end
							endcase
						end
						SYS_EVAL_EVLIS_SETUP: begin
							stack <= mem_addr; // Save the stack from all the previous operations
							
							regs[0] <= data_regs[0][9:0];
							regs[1] <= regs[1];
							regs[2] <= 0;
							regs[3] <= 0;
							
							state <= SYS_EVLIS_INIT;
							sys_func <= SYS_FUNC_EVLIS;
						end
						SYS_EVAL_EVLIS_CONT: begin
							mem_addr0 <= regs[0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_EVAL_EVLIS_CONT_WAIT;
						end
						SYS_EVAL_EVLIS_CONT_WAIT: begin
							if(mem_ready) begin
								regs[0] <= mem_data[19:10];
								regs[2] <= regs[1];
								regs[1] <= result;
								regs[3] <= 0;
								
								state <= SYS_APPLY_INIT;
								sys_func <= SYS_FUNC_APPLY;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVAL_QUOTE_WAIT: begin
							if(mem_ready) begin
								state <= SYS_RETURN_INIT;
								sys_func <= SYS_FUNC_RETURN;
								result <= mem_data[19:10];
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVAL_IF_FETCH_0: begin
							if(mem_ready) begin
								regs[0] <= mem_data[19:10];
								
								mem_addr0 <= mem_data[9:0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVAL_IF_FETCH_1;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVAL_IF_FETCH_1: begin
							if(mem_ready) begin
								regs[1] <= mem_data[19:10];
								
								mem_addr0 <= mem_data[9:0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVAL_IF_FETCH_2;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVAL_IF_FETCH_2: begin
							if(mem_ready) begin
								regs[2] <= mem_data[19:10];
								
								state <= SYS_EVIF_INIT;
								sys_func <= SYS_FUNC_EVIF;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_APPLY: begin
					case(state)
						SYS_APPLY_INIT: begin
							mem_addr0 <= regs[0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_APPLY_GET_SECOND_ARG;
						end
						SYS_APPLY_GET_SECOND_ARG: begin
							if(mem_ready) begin
								data_regs[0] <= mem_data;
								
								mem_addr0 <= regs[1];
								mem_func = `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_STORE_ARGS;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_STORE_ARGS: begin
							if(mem_ready) begin
								data_regs[1] <= mem_data;

								state <= SYS_APPLY_IS_ATOM;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_IS_ATOM: begin
							// if is atom
							if(data_regs[0][23:20] != 4'h0) begin
								if(data_regs[0][9:0] < 10'h2) begin
									mem_addr0 <= regs[1];
									mem_func <= `GET_CONTENTS;
									mem_execute <= 1;
									
									state <= SYS_APPLY_ARITH_0;
								end
								else begin
									case(data_regs[0][9:0])
										OP_CAR: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_OP_CAR_0;
										end
										OP_CDR: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_OP_CDR_0;
										end
										OP_CONS: begin
											mem_addr0 <= data_regs[1][9:0];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_OP_CONS_0;
										end
										OP_EQ: begin
											mem_addr0 <= data_regs[1][9:0];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_OP_EQ_CALL;
										end
										OP_ATOM: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func = `GET_CONTENTS;
											mem_execute <= 1;
										
											state <= SYS_APPLY_OP_ATOM;
										end
										OP_LESS: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_COMPARE_0;
										end
										OP_EQUAL: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_COMPARE_0;
										end
										OP_GREATER: begin
											mem_addr0 <= data_regs[1][19:10];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_COMPARE_0;
										end
										OP_AND: begin
											mem_addr0 <= data_regs[1][9:0];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_LOGIC_0;
										end
										OP_OR: begin
											mem_addr0 <= data_regs[1][9:0];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_LOGIC_0;
										end
										OP_NOT: begin
											mem_addr0 <= data_regs[1][9:0];
											mem_func <= `GET_CONTENTS;
											mem_execute <= 1;
											
											state <= SYS_APPLY_LOGIC_0;
										end
										default: begin
											// SYS_APPLY_1
											calling_func <= SYS_APPLY_1;
											arg_push_count <= 3;
											
											sys_func <= SYS_FUNC_PUSH;
											state <= SYS_PUSH_INIT;
										end
									endcase
								end
							end
							else begin
								calling_func <= SYS_APPLY_2;
								arg_push_count <= 3;
								
								sys_func <= SYS_FUNC_PUSH;
								state <= SYS_PUSH_INIT;
							end
						end
						SYS_APPLY_ARITH_0: begin
							if(mem_ready) begin
								data_regs[1] <= mem_data;
								
								mem_addr0 <= mem_data[19:10];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_ARITH_1;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_ARITH_1: begin
							if(mem_ready) begin
								// addition
								if(data_regs[0][9:0] == OP_ADD) begin
									mem_addr1 <= 0;
								end
								// subtraction
								else begin
									mem_addr1 <= mem_data[9:0];
								end
								
								mem_addr0 <= 0;
								mem_type_info <= CELL_NUMBER;
								mem_func <= `GET_CONS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_ARITH_WAIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_ARITH_WAIT: begin
							if(mem_ready) begin
								if(data_regs[0][9:0] == OP_ADD) begin
									regs[1] <= regs[1];
//									
//									sys_func <= SYS_FUNC_EVARTH;
//									state <= SYS_EVARTH_INIT;
								end
								// subtraction
								else begin
									regs[1] <= data_regs[1][9:0];
//									mem_func <= `GET_CONTENTS;
//									mem_execute <= 1;
//									
//									state <= SYS_APPLY_ARITH_WAIT_1;
								end
								
								regs[0] <= regs[0];
								regs[2] <= mem_addr;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVARTH;
								state <= SYS_EVARTH_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_type_info <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_ARITH_WAIT_1: begin
							if(mem_ready) begin
								regs[1] <= mem_data[9:0];
								
								sys_func <= SYS_FUNC_EVARTH;
								state <= SYS_EVARTH_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_CAR_0: begin
							if(mem_ready) begin
								result <= mem_data[19:10];
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_CDR_0: begin
							if(mem_ready) begin
								result <= mem_data[9:0];
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_CONS_0: begin
							if(mem_ready) begin
								mem_addr0 <= data_regs[1][19:10];
								mem_addr1 <= mem_data[19:10];
								mem_type_info <= CELL_GENERAL;
								mem_func <= `GET_CONS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_OP_CONS_RETURN;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_CONS_RETURN: begin
							if(mem_ready) begin
								result <= mem_addr;
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_EQ_CALL: begin
							if(mem_ready) begin
								regs[0] <= data_regs[1][19:10];
								regs[1] <= mem_data[19:10];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EQ;
								state <= SYS_EQ_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_OP_ATOM: begin
							if(mem_ready) begin
								if(mem_data[23:20] != 0) begin
									result <= 1;
								end
								else begin
									result <= 0;
								end
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_COMPARE_0: begin
							if(mem_ready) begin
								data_regs[2] <= mem_data;
								
								mem_addr0 <= data_regs[1][9:0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_COMPARE_1;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_COMPARE_1: begin
							if(mem_ready) begin
								mem_addr0 <= mem_data[19:10];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_COMPARE_EXECUTE;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						// At this point...
						// data_regs[0] = contents(regs[0])
						// data_regs[1] = contents(regs[1])
						// data_regs[2] = contents(regs[1]->car)
						// mem_data = contents(regs[1]->cdr->car)
						SYS_APPLY_COMPARE_EXECUTE: begin
							if(mem_ready) begin
								case(data_regs[0][9:0])
									OP_LESS: begin
										if(data_regs[2][19:10] < mem_data[19:10]) begin
											result <= 1;
										end
										else begin
											result <= 0;
										end
									end
									OP_EQUAL: begin
										if(data_regs[2][19:10] == mem_data[19:10]) begin
											result <= 1;
										end
										else begin
											result <= 0;
										end
									end
									OP_GREATER: begin
										if(data_regs[2][19:10] > mem_data[19:10]) begin
											result <= 1;
										end
										else begin
											result <= 0;
										end
									end
									default:;
								endcase
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_LOGIC_0: begin
							if(mem_ready) begin
								case(data_regs[0][9:0])
									OP_AND: begin
										if(data_regs[1][19:10] == 10'b1 && mem_data[19:10] == 10'b1) begin
											result <= 1;
										end
										else begin
											result <= 0;
										end
									end
									OP_OR: begin
										if(data_regs[1][19:10] == 10'b1 || mem_data[19:10] == 10'b1) begin
											result <= 1;
										end
										else begin
											result <= 0;
										end
									end
									OP_NOT: begin
										if(data_regs[1][19:10] == 1) begin
											result <= 0;
										end
										else begin
											result <= 1;
										end
									end
									default:;
								endcase
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_CALL_EVAL: begin
							regs[0] <= regs[0];
							regs[1] <= regs[2];
							regs[2] <= 0;
							regs[3] <= 0;
							
							sys_func <= SYS_FUNC_EVAL;
							state <= SYS_EVAL_INIT;
						end
						SYS_APPLY_CALL_APPLY: begin
							regs[0] <= result;
							regs[1] <= regs[1];
							regs[2] <= regs[2];
							regs[3] <= 0;
							
							sys_func <= SYS_FUNC_APPLY;
							state <= SYS_APPLY_INIT;
						end
						SYS_APPLY_CALL_CONENV: begin
							mem_addr0 <= data_regs[0][9:0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_APPLY_CALL_CONENV_1;
						end
						SYS_APPLY_CALL_CONENV_1: begin
							if(mem_ready) begin
								regs[0] <= mem_data[19:10];
								regs[1] <= regs[1];
								regs[2] <= regs[2];
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_CONENV;
								state <= SYS_CONENV_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_CALL_EVAL_1: begin
							mem_addr0 <= data_regs[0][9:0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_APPLY_CALL_EVAL_2;
						end
						SYS_APPLY_CALL_EVAL_2: begin
							if(mem_ready) begin
								mem_addr0 <= mem_data[9:0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_APPLY_CALL_EVAL_3;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_APPLY_CALL_EVAL_3: begin
							if(mem_ready) begin
								regs[0] <= mem_data[19:10];
								regs[1] <= result;
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVAL;
								state <= SYS_EVAL_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_EVLIS: begin
					case(state)
						SYS_EVLIS_INIT: begin
							if(regs[0] == 0) begin
								result <= 0;
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								calling_func <= SYS_EVLIS_0;
								arg_push_count <= 2;
								
								sys_func <= SYS_FUNC_PUSH;
								state <= SYS_PUSH_INIT;
							end
						end
						SYS_EVLIS_CALL_EVAL_0: begin
							mem_addr0 <= regs[0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_EVLIS_CALL_EVAL_1;
						end
						SYS_EVLIS_CALL_EVAL_1: begin
							if(mem_ready) begin
								regs[0] <= mem_data[19:10];
								regs[1] <= regs[1];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVAL;
								state <= SYS_EVAL_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVLIS_PUSH_TWO: begin
							calling_func <= SYS_EVLIS_1;
							regs[2] <= result;
							arg_push_count <= 3;
							
							sys_func <= SYS_FUNC_PUSH;
							state <= SYS_PUSH_INIT;
						end
						SYS_EVLIS_CALL_EVLIS_0: begin
							mem_addr0 <= regs[0];
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_EVLIS_CALL_EVLIS_1;
						end
						SYS_EVLIS_CALL_EVLIS_1: begin
							if(mem_ready) begin
								regs[0] <= mem_data[9:0];
								regs[1] <= regs[1];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVLIS;
								state <= SYS_EVLIS_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVLIS_GET_CONS: begin
							mem_addr0 <= regs[2];
							mem_addr1 <= result;
							mem_type_info <= CELL_GENERAL;
							mem_func <= `GET_CONS;
							mem_execute <= 1;
							
							state <= SYS_EVLIS_RETURN;
						end
						SYS_EVLIS_RETURN: begin
							if(mem_ready) begin
								result <= mem_addr;
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_EVIF: begin
					case(state)
						SYS_EVIF_INIT: begin
							calling_func <= SYS_EVIF;
							arg_push_count <= 4;
							
							sys_func <= SYS_FUNC_PUSH;
							state <= SYS_PUSH_INIT;
						end
						SYS_EVIF_CALL_EVAL: begin
							regs[0] <= regs[0];
							regs[1] <= regs[3];
							regs[2] <= 0;
							regs[3] <= 0;
							
							sys_func <= SYS_FUNC_EVAL;
							state <= SYS_EVAL_INIT;
						end
						SYS_EVIF_CHOOSE: begin
							if(result == 0) begin
								regs[0] <= regs[1];
								regs[1] <= regs[3];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVAL;
								state <= SYS_EVAL_INIT;
							end
							else begin
								regs[0] <= regs[2];
								regs[1] <= regs[3];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_EVAL;
								state <= SYS_EVAL_INIT;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_EVARTH: begin
					case(state)
						SYS_EVARTH_INIT: begin
							if(regs[1] == 0) begin
								result <= regs[2];
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= regs[0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVARTH_GET_0;
							end
						end
						SYS_EVARTH_GET_0: begin
							if(mem_ready) begin
								data_regs[0] <= mem_data;
							
								mem_addr0 <= regs[1];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVARTH_GET_1;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVARTH_GET_1: begin
							if(mem_ready) begin
								data_regs[1] <= mem_data;
							
								mem_addr0 <= regs[2];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVARTH_GET_1_CAR;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVARTH_GET_1_CAR: begin
							if(mem_ready) begin
								data_regs[2] <= mem_data;
							
								mem_addr0 <= data_regs[1][19:10];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_EVARTH_RECURSE;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVARTH_RECURSE: begin
							if(mem_ready) begin
								case(data_regs[0][7:0])
									OP_ADD: begin
										mem_addr1 <= data_regs[2][9:0] + mem_data[9:0];
									end
									OP_SUB: begin
										mem_addr1 <= data_regs[2][9:0] - mem_data[9:0];
									end
									default:;
								endcase
								
								mem_addr0 <= 0;
								mem_type_info <= CELL_NUMBER;
								mem_func <= `GET_CONS;
								mem_execute <= 1;
								
								state <= SYS_EVARTH_CONS_WAIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_EVARTH_CONS_WAIT: begin
							if(mem_ready) begin
								regs[0] <= regs[0];
								regs[1] <= data_regs[1][9:0];
								regs[2] <= mem_addr;
								regs[3] <= regs[3];
								
								sys_func <= SYS_FUNC_EVARTH;
								state <= SYS_EVARTH_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_type_info <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_CONENV: begin
					case(state)
						SYS_CONENV_INIT: begin
							if(regs[0] == 0) begin
								result <= regs[2];
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= regs[0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_CONENV_GET_ARG;
							end
						end
						SYS_CONENV_GET_ARG: begin
							if(mem_ready) begin
								data_regs[0] <= mem_data;
								
								mem_addr0 <= regs[1];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_CONENV_GET_CONS;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_CONENV_GET_CONS: begin
							if(mem_ready) begin
								data_regs[1] <= mem_data;
							
								mem_addr0 <= data_regs[0][19:10];
								mem_addr1 <= mem_data[19:10];
								mem_type_info <= CELL_GENERAL;
								mem_func <= `GET_CONS;
								mem_execute <= 1;
								
								state <= SYS_CONENV_PUSH_ARGS;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_CONENV_PUSH_ARGS: begin
							if(mem_ready) begin
								regs[3] <= mem_addr;
								
								calling_func <= SYS_CONENV;
								arg_push_count <= 4;
								
								sys_func <= SYS_FUNC_PUSH;
								state <= SYS_PUSH_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_CONENV_CALL_CONENV: begin
							regs[0] <= data_regs[0][9:0];
							regs[1] <= data_regs[1][9:0];
							regs[2] <= regs[2];
							regs[3] <= 0;
							
							sys_func <= SYS_FUNC_CONENV;
							state <= SYS_CONENV_INIT;
						end
						SYS_CONENV_RETURN: begin
							mem_addr0 <= regs[3];
							mem_addr1 <= result;
							mem_type_info <= CELL_GENERAL;
							mem_func <= `GET_CONS;
							mem_execute <= 1;
							
							state <= SYS_CONENV_DONE;
						end
						SYS_CONENV_DONE: begin
							if(mem_ready) begin
								result <= mem_addr;
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_LOOKUP: begin
					case(state)
						SYS_LOOKUP_INIT: begin
							// Symbol wasn't found
							if(regs[1] == 0) begin
								sys_func <= SYS_FUNC_DONE;
								state <= 0;
							end
							// Else, begin checking if symbols are eq
							else begin
								calling_func <= SYS_LOOKUP;
								arg_push_count <= 2;
								
								sys_func <= SYS_FUNC_PUSH;
								state <= SYS_PUSH_INIT;
							end
						end
						// Setup regs for the call to eq
						SYS_LOOKUP_SETUP_REGS_0: begin
							state <= SYS_LOOKUP_SETUP_REGS_1;
						
							mem_addr0 <= regs[1];
							mem_execute <= 1;
							mem_func <= `GET_CONTENTS;
						end
						SYS_LOOKUP_SETUP_REGS_1: begin
							if(mem_ready) begin
								state <= SYS_LOOKUP_SETUP_REGS_2;
							
								data_regs[1] <= mem_data; // Store for later
								mem_addr0 <= mem_data[19:10];
								mem_execute <= 1;
								mem_func <= `GET_CONTENTS;
							end
							else begin
								mem_addr0 <= 0;
								mem_execute <= 0;
								mem_func <= 0;
							end
						end
						SYS_LOOKUP_SETUP_REGS_2: begin
							if(mem_ready) begin
								sys_func <= SYS_FUNC_EQ;
								state <= SYS_EQ_INIT;
							
								data_regs[0] <= mem_data; // Store for later
								regs[1] <= regs[0];
								regs[0] <= mem_data[9:0];
							end
							else begin
								mem_addr0 <= 0;
								mem_execute <= 0;
								mem_func <= 0;
							end
						end
						SYS_LOOKUP_CHECK_EQ: begin
							if(result) begin
								result <= data_regs[0][9:0];
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
							else begin
								regs[0] <= regs[0];
								regs[1] <= data_regs[1][9:0];
								regs[2] <= 0;
								regs[3] <= 0;
								
								sys_func <= SYS_FUNC_LOOKUP;
								state <= SYS_LOOKUP_INIT;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_RETURN: begin
					case(state)
						SYS_RETURN_INIT: begin
							mem_addr0 <= stack;
							mem_func <= `GET_CONTENTS;
							mem_execute <= 1;
							
							state <= SYS_RETURN_RESTORE_CALLING;
						end
						SYS_RETURN_RESTORE_CALLING: begin
							if(mem_ready) begin
								calling_func <= mem_data[13:10];
								stack <= mem_data[9:0];
								
								mem_addr0 <= mem_data[9:0];
								mem_func <= `GET_CONTENTS;
								mem_execute <= 1;
								
								state <= SYS_RETURN_POP_STACK;
								counter <= 0;
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_RETURN_POP_STACK: begin
							if(mem_ready) begin
								// If there is no stack left, execution is complete.
								if(mem_data == 0) begin
									sys_func <= SYS_FUNC_DONE;
									state <= 0;
								end
								// We reached the end of this frame
								else if(mem_data[23:20] == CELL_RETURN) begin
									state <= SYS_RETURN_DONE;
								end
								// We have more to pop and store in the regs
								else begin
									regs[counter] <= mem_data[19:10];
									stack <= mem_data[9:0];
									counter <= counter + 4'b1;
									
									mem_addr0 <= mem_data[9:0];
									mem_func <= `GET_CONTENTS;
									mem_execute <= 1;
								end
							end
							else begin
								mem_addr0 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_RETURN_DONE: begin
							case(calling_func)
								SYS_EVAL_0: begin
									sys_func <= SYS_FUNC_EVAL;
									state <= SYS_EVAL_EVLIS_CONT;
								end
								SYS_EVAL_1: begin
									sys_func <= SYS_FUNC_EVAL;
									state <= 0; // TODO FIX
								end
								SYS_APPLY_0:;
								SYS_APPLY_1: begin
									sys_func <= SYS_FUNC_APPLY;
									state <= SYS_APPLY_CALL_APPLY;
								end
								SYS_APPLY_2: begin
									sys_func <= SYS_FUNC_APPLY;
									state <= SYS_APPLY_CALL_EVAL_1;
								end
								SYS_EVLIS_0: begin
									sys_func <= SYS_FUNC_EVLIS;
									state <= SYS_EVLIS_PUSH_TWO;
								end
								SYS_EVLIS_1: begin
									sys_func <= SYS_FUNC_EVLIS;
									state <= SYS_EVLIS_GET_CONS;
								end
								SYS_EVIF: begin
									sys_func <= SYS_FUNC_EVLIS;
									state <= SYS_EVIF_CHOOSE;
								end
								SYS_EVBEGIN:;
								SYS_CONENV: begin
									sys_func <= SYS_FUNC_CONENV;
									state <= SYS_CONENV_RETURN;
								end
								SYS_LOOKUP: begin
									sys_func <= SYS_FUNC_LOOKUP;
									state <= SYS_LOOKUP_SETUP_REGS_0;
								end
								SYS_EQ: begin
									sys_func <= SYS_FUNC_LOOKUP;
									state <= SYS_LOOKUP_CHECK_EQ;
								end
								SYS_REPL: begin
									sys_func <= SYS_FUNC_DONE;
									state <= 0;
								end
								default:;
							endcase
						end
					endcase
				end
				SYS_FUNC_PUSH: begin
					case(state)
						SYS_PUSH_INIT: begin
							// Catch the case when the push_arg_count == 0 upon the SYS_FUNC_PUSH call
							if(arg_push_count != 0) begin
								mem_type_info <= CELL_GENERAL;
								mem_addr0 <= regs[arg_push_count - 2'b1];
								mem_addr1 <= stack;
								mem_func <= `GET_CONS;
								mem_execute <= 1;
								state <= SYS_PUSH_WAIT_CONS;
							end
							else begin
								state <= SYS_PUSH_LAST_WAIT;
							end
						end
						SYS_PUSH_WAIT_CONS: begin
							if(mem_ready) begin
								arg_push_count <= arg_push_count - 2'b1;
								stack <= mem_addr;
								
								// If arg_push_count will be 0 next clock, move on
								if(arg_push_count == 1) begin
									state <= SYS_PUSH_LAST_PART;
								end
								// Else we have more arguments to push. Do so
								else begin
									state <= SYS_PUSH_INIT;
								end
							end
							else begin
								mem_type_info <= 0;
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						SYS_PUSH_LAST_PART: begin
							mem_type_info <= CELL_RETURN;
							mem_addr0 <= {6'b0, calling_func};
							mem_addr1 <= stack;
							mem_func <= `GET_CONS;
							mem_execute <= 1;
							
							state <= SYS_PUSH_LAST_WAIT;
						end
						SYS_PUSH_LAST_WAIT: begin
							// TODO figure out what this does
							if(mem_ready) begin
								stack <= mem_addr;
								
								// Continue executing the previous system function
								// before the call to SYS_PUSH
								case(calling_func)
									SYS_EVAL_0: begin
										sys_func <= SYS_FUNC_EVAL;
										state <= SYS_EVAL_EVLIS_SETUP;
									end
									SYS_LOOKUP: begin
										sys_func <= SYS_FUNC_LOOKUP;
										state <= SYS_LOOKUP_SETUP_REGS_0;
									end
									SYS_APPLY_1: begin
										sys_func <= SYS_FUNC_APPLY;
										state <= SYS_APPLY_CALL_EVAL;
									end
									SYS_APPLY_2: begin
										sys_func <= SYS_FUNC_APPLY;
										state <= SYS_APPLY_CALL_CONENV;
									end
									SYS_CONENV: begin
										sys_func <= SYS_FUNC_CONENV;
										state <= SYS_CONENV_CALL_CONENV;
									end
									SYS_EVLIS_0: begin
										sys_func <= SYS_FUNC_EVLIS;
										state <= SYS_EVLIS_CALL_EVAL_0;
									end
									SYS_EVLIS_1: begin
										sys_func <= SYS_FUNC_EVLIS;
										state <= SYS_EVLIS_CALL_EVLIS_0;
									end
									SYS_EVIF: begin
										sys_func <= SYS_FUNC_EVLIS;
										state <= SYS_EVIF_CALL_EVAL;
									end
									default:;
								endcase
							end
							else begin
								mem_type_info <= 0;
								mem_addr0 <= 0;
								mem_addr1 <= 0;
								mem_func <= 0;
								mem_execute <= 0;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_EQ: begin
					case(state)
						SYS_EQ_INIT: begin
							mem_addr0 <= regs[0];
							mem_execute <= 1;
							mem_func <= `GET_CONTENTS;
							
							state <= SYS_EQ_FETCH_FIRST;
						end
						SYS_EQ_FETCH_FIRST: begin
							// Fetch reg[0]
							if(mem_ready) begin
								data_regs[2] <= mem_data; // regs 0 and 1 are already in use by lookup

								mem_addr0 <= regs[1];
								mem_execute <= 1;
								mem_func <= `GET_CONTENTS;
								
								state <= SYS_EQ_FETCH_SECOND;
							end
							else begin
								mem_addr0 <= 0;
								mem_execute <= 0;
								mem_func <= 0;
							end
						end
						// Fetch reg[1]
						SYS_EQ_FETCH_SECOND: begin
							if(mem_ready) begin
								data_regs[3] <= mem_data;
								state <= SYS_EQ_CHECK;
							end
							else begin
								mem_addr0 <= 0;
								mem_execute <= 0;
								mem_func <= 0;
							end
						end
						SYS_EQ_CHECK: begin
							// Get cells are of the right type
							if(data_regs[2] >> 20 != CELL_SYMBOL || data_regs[3] >> 20 != CELL_SYMBOL) begin
								sys_func <= SYS_FUNC_DONE;
								state <= 0;
							end
							else if(data_regs[2][19:10] == data_regs[3][19:10]) begin
							
								// If both symbols are done, they are equal
								if(data_regs[2][9:0] == 0 && data_regs[3][9:0] == 0) begin
									result <= 1;
									
									sys_func <= SYS_FUNC_RETURN;
									state <= SYS_RETURN_INIT;
								end
								// If one is done before the other, they are not equal
								else if(data_regs[2][9:0] == 0 ^ data_regs[3][9:0] == 0) begin
									result <= 0;
									
									sys_func <= SYS_FUNC_RETURN;
									state <= SYS_RETURN_INIT;
								end
								// If they are both no done, recursively call eq again
								else begin
									regs[0] <= data_regs[2][9:0];
									regs[1] <= data_regs[3][9:0];
									regs[2] <= 0;
									regs[3] <= 0;
									
									sys_func <= SYS_FUNC_EQ;
									state <= SYS_EQ_INIT;
								end
							end
							else begin
								result <= 0;
								
								sys_func <= SYS_FUNC_RETURN;
								state <= SYS_RETURN_INIT;
							end
						end
						default:;
					endcase
				end
				SYS_FUNC_DONE: begin
				
				end
				default:;
			endcase
		end
		else if(!mem_powered_up) begin
			if(mem_ready) begin
				mem_powered_up <= 1;
			end
		end
	end

endmodule
