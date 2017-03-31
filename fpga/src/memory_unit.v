/*
 Will have 4 main functions used by external modules.
  - Given the address of a cell, return the address of its car
  - Given the address of a cell, return the address of its cdr
  - Given the address of two cells, return a new cell with them consed together
  - Given the address of a cell, return the contents on that cell
*/

`include "memory_unit.vh"

module memory_unit(func, execute, addr0, addr1, type_info, addr_out, data_out, is_ready, power, clk, rst);

	input power, clk, rst;

	// Control wires for this module
	input [1:0] func;
	input execute;
	input [`memory_addr_width - 1:0] addr0, addr1;
	input [3:0] type_info;
	
	// Signal wires for this module
	reg is_ready_reg;
	output wire is_ready;
	assign is_ready = !execute && is_ready_reg;
	output reg [`memory_addr_width - 1:0] addr_out;
	output reg [`memory_data_width - 1:0] data_out;
	
	// Interface with the ram module
	reg [`memory_addr_width - 1:0] mem_addr;
	reg mem_write;
	reg [`memory_data_width - 1:0] mem_data_in;
	wire [`memory_data_width - 1:0] mem_data_out;
	
	// Internal regs and wires
	reg need_car_or_cdr; // 0 for car, 1 for cdr
	reg should_fetch_data;
	reg [`memory_addr_width - 1:0] free_mem;
	reg [3:0] state;
	
	// States
	parameter STATE_INIT_SETUP 	= 4'h0,
				 STATE_INIT_WAIT_0	= 4'h1,
				 STATE_INIT_STORE_FREE_MEM 	= 4'h2,
				 STATE_INIT_CLEAR_NIL = 4'h3,
				 STATE_INIT_WAIT_1	= 4'h4,
				 STATE_WAIT 			= 4'h5,
				 STATE_READ_WAIT_0	= 4'h6,
				 STATE_READ_WAIT_1	= 4'h7,
				 STATE_READ_FINISH	= 4'h8,
				 STATE_CONS_WAIT 		= 4'h9;
				 
	ram ram(.address (mem_addr),
			  .clock (clk),
			  .data (mem_data_in),
			  .wren (mem_write),
			  .q (mem_data_out));
	
	always@(posedge clk or negedge rst) begin
		if(!rst) begin
			state <= STATE_INIT_SETUP;
			
			free_mem <= 0;
			
			mem_addr <= 0;
			mem_data_in <= 0;
			
			is_ready_reg <= 0;
			need_car_or_cdr <= 0;
			should_fetch_data <= 0;
		end
		else if (power) begin
			case (state)
				// Initialize the free memory register
				STATE_INIT_SETUP: begin
					state <= STATE_INIT_WAIT_0;
					mem_addr <= 0;
				end
				STATE_INIT_WAIT_0: begin
					state <= STATE_INIT_STORE_FREE_MEM;
				end
				// Record the start of the free memory store
				STATE_INIT_STORE_FREE_MEM: begin
					state <= STATE_INIT_CLEAR_NIL;
					free_mem <= mem_data_out[9:0];
				end
				// Clear the nil pointer
				STATE_INIT_CLEAR_NIL: begin
					state <= STATE_INIT_WAIT_1;
					
					mem_addr <= 0;
					mem_data_in <= 0;
					mem_write <= 1;
				end
				STATE_INIT_WAIT_1: begin
					state <= STATE_WAIT;
					
					mem_write <= 0;
				end
				// Wait for a command dispatch
				STATE_WAIT: begin
					if(execute) begin
						is_ready_reg <= 0;
						// Dispatch according to the function
						case (func)
							`GET_CAR: begin
								need_car_or_cdr <= 0;
								should_fetch_data <= 0;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
							`GET_CDR: begin
								need_car_or_cdr <= 1;
								should_fetch_data <= 0;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
							`GET_CONS: begin
								mem_data_in <= {type_info, addr0, addr1};
								mem_addr <= free_mem;
								mem_write <= 1;
								state <= STATE_CONS_WAIT;
							end
							`GET_CONTENTS: begin
								should_fetch_data <= 1;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
						endcase
					end
					// Keep waiting
					else begin
						state <= STATE_WAIT;
						is_ready_reg <= 1;
						mem_addr <= 0;
					end
				end
				// Various wait states used when reading from memory
				STATE_READ_WAIT_0: begin
					state <= STATE_READ_WAIT_1;
				end
				STATE_READ_WAIT_1: begin
					state <= STATE_READ_FINISH;
				end
				STATE_READ_FINISH: begin
					state <= STATE_WAIT;
					is_ready_reg <= 1;
					
					// Output the correct data
					if(should_fetch_data) begin
						data_out <= mem_data_out;
					end
					else begin
						if(need_car_or_cdr) begin
							addr_out <= mem_data_out[9:0];
						end
						else begin
							addr_out <= mem_data_out[19:10];
						end
					end
				end
				// Return a new cell
				STATE_CONS_WAIT: begin
					mem_write <= 0;
					addr_out <= free_mem;
					free_mem <= free_mem + `memory_addr_width'b1;
					
					is_ready_reg <= 1;
					
					state <= STATE_WAIT;
				end
				default:;
			endcase
		end
	end

endmodule
