
`include "memory_unit.vh"

module eval_unit(mem_ready, mem_func, mem_execute, mem_addr0, mem_addr1, mem_type_info, mem_addr, mem_data, power, clk, rst, sys_func, state);

	input power, clk, rst;
	
	reg mem_powered_up;
	
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
	reg [`memory_addr_width - 1:0] result;
	reg [`memory_data_width - 1:0] data_regs [0:3];
	
	reg [`memory_addr_width - 1:0] stack;
	reg [3:0] calling_func;
	reg [1:0] arg_push_count;
	reg [`memory_addr_width - 1:0] push_regs [0:3];
	
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
	parameter OP_ADD			= 8'h0,
				 OP_SUB			= 8'h1,
				 OP_LESS			= 8'h2,
				 OP_EQUAL		= 8'h3,
				 OP_GREATER		= 8'h4,
				 OP_AND			= 8'h5,
				 OP_OR			= 8'h6,
				 OP_NOT			= 8'h7,
				 OP_CAR			= 8'h8,
				 OP_CDR			= 8'h9,
				 OP_CONS			= 8'hA,
				 OP_EQ			= 8'hB,
				 OP_ATOM			= 8'hC,
				 OP_IF			= 8'hD,
				 OP_LAMBDA		= 8'hE,
				 OP_QUOTE		= 8'hF,
				 OP_DEFINE		= 8'h10,
				 OP_BEGIN		= 8'h12;
				 
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
				 SYS_REPL		= 4'hC;
	
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
				 SYS_FUNC_ERROR	= 4'hA;
				 
	// Eval states
	parameter SYS_EVAL_INIT 			= 4'h0,
				 SYS_EVAL_FETCH_0_WAIT	= 4'h1,
				 SYS_EVAL_FIRST_IF		= 4'h2,
				 SYS_EVAL_FETCH_1_WAIT	= 4'h3,
				 SYS_EVAL_SWITCH			= 4'h4,
				 SYS_EVAL_QUOTE_WAIT		= 4'h5,
				 SYS_EVAL_IF_FETCH_0		= 4'h6,
				 SYS_EVAL_IF_FETCH_1		= 4'h7,
				 SYS_EVAL_IF_FETCH_2		= 4'h8,
				 SYS_EVAL_PUSH_0			= 4'h9,
				 SYS_EVAL_PUSH_1			= 4'hA,
				 SYS_EVAL_EVLIS_SETUP	= 4'hB,
				 SYS_EVAL_EVLIS_CONT 	= 4'hC,
				 SYS_EVAL_EVLIS_CONT_WAIT = 4'hD;
				 
	// Apply states
	parameter SYS_APPLY_INIT			= 4'h0;
	
	// Evlis states
	parameter SYS_EVLIS_INIT			= 4'h0;
	
	// Evif states
	parameter SYS_EVIF_INIT				= 4'h0;
	
	// Evarth states
	parameter SYS_EVARTH_INIT			= 4'h0;
	
	// Conenv states
	parameter SYS_CONENV_INIT			= 4'h0;
	
	// Lookup states
	parameter SYS_LOOKUP_INIT			= 4'h0,
				 SYS_LOOKUP_SETUP_REGS_0 = 4'h1,
				 SYS_LOOKUP_SETUP_REGS_1 = 4'h2,
				 SYS_LOOKUP_SETUP_REGS_2 = 4'h3,
				 SYS_LOOKUP_CHECK_EQ		= 4'h4;
	
	// Return states
	parameter SYS_RETURN_INIT					= 4'h0,
				 SYS_RETURN_RESTORE_CALLING	= 4'h1,
				 SYS_RETURN_POP_STACK			= 4'h2,
				 SYS_RETURN_DONE					= 4'h3;
				 
	// Push states
	parameter SYS_PUSH_INIT				= 4'h0,
				 SYS_PUSH_WAIT_CONS		= 4'h1,
				 SYS_PUSH_LAST_PART		= 4'h2,
				 SYS_PUSH_LAST_WAIT		= 4'h3;
				 
	// Eq states
	parameter SYS_EQ_INIT			= 4'h0,
				 SYS_EQ_FETCH_FIRST	= 4'h1,
				 SYS_EQ_FETCH_SECOND = 4'h2,
				 SYS_EQ_CHECK			= 4'h3;
	
	// System push
	parameter SYS_PUSH_INIT				= 4'h0,
				 SYS_PUSH_0					= 4'h1;
	
	output reg [3:0] sys_func; // Records the current system function
	output reg [3:0] state; // Records the state inside the current system function
	
	always@(posedge clk or negedge rst) begin
		if(!rst) begin
			mem_powered_up <= 0;
			
			sys_func <= SYS_FUNC_EVAL;
			state <= SYS_EVAL_INIT;
			
			regs[0] <= 1;
			regs[1] <= 0;
			regs[2] <= 0;
			regs[3] <= 0;
			
			stack <= 0;
		end
		else if(power && mem_powered_up) begin
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
				SYS_FUNC_APPLY:;
				SYS_FUNC_EVLIS:;
				SYS_FUNC_EVIF:;
				SYS_FUNC_EVARTH:;
				SYS_FUNC_CONENV:;
				SYS_FUNC_LOOKUP: begin
					case(state)
						SYS_LOOKUP_INIT: begin
							// Symbol wasn't found
							if(regs[1] == 0) begin
								sys_func <= SYS_FUNC_ERROR;
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
								
								mem_addr0 <= stack;
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
								// We reached the end of this frame
								if(mem_data[23:20] == CELL_RETURN || mem_data == 0) begin
									state <= SYS_RETURN_DONE;
								end
								// We have more to pop and store in the regs
								else begin
									regs[counter] <= mem_data[19:10];
									stack <= mem_data[9:0];
									counter <= counter + 4'b1;
									
									mem_addr0 <= stack;
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
							state <= 0;
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
								SYS_APPLY_1:;
								SYS_APPLY_2:;
								SYS_EVLIS_0:;
								SYS_EVLIS_1:;
								SYS_EVIF:;
								SYS_EVBEGIN:;
								SYS_CONENV:;
								SYS_LOOKUP: begin
									sys_func <= SYS_FUNC_LOOKUP;
									state <= SYS_LOOKUP_SETUP_REGS_0;
								end
								SYS_EQ: begin
									sys_func <= SYS_FUNC_LOOKUP;
									state <= SYS_LOOKUP_CHECK_EQ;
								end
								SYS_RETURN:;
								SYS_REPL:;
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
							mem_addr0 <= {20'b0, calling_func};
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
								sys_func <= SYS_FUNC_ERROR;
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
				SYS_FUNC_ERROR:;
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
