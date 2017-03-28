/*
 Will have 4 main functions used by external modules.
  - Given the address of a cell, return the address of its car
  - Given the address of a cell, return the address of its cdr
  - Given the address of two cells, return a new cell with them consed together
  - Given the address of a cell, return the contents on that cell
*/

module memory_unit(func, execute, addr0, addr1, addr_out, data_out, is_ready, clk, rst, state, free_mem, mem_addr, mem_data_out);

	input clk, rst;

	// Control wires for this module
	input [1:0] func;
	input execute;
	input [`memory_addr_width - 1:0] addr0, addr1;
	
	// Signal wires for this module
	output reg is_ready;
	output reg [`memory_addr_width - 1:0] addr_out;
	output reg [`memory_data_width - 1:0] data_out;
	
	// Interface with the ram module
	output reg [`memory_addr_width - 1:0] mem_addr;
	reg mem_write;
	reg [`memory_data_width - 1:0] mem_data_in;
	output wire [`memory_data_width - 1:0] mem_data_out;
	
	// Internal regs and wires
	reg need_car_or_cdr; // 0 for car, 1 for cdr
	reg should_fetch_data;
	output reg [`memory_addr_width - 1:0] free_mem;
	output reg [3:0] state;
	
	// Functions
	parameter GET_CAR			= 2'b00,
				 GET_CDR 		= 2'b01,
				 GET_CONS 		= 2'b10,
				 GET_CONTENTS 	= 2'b11;
	
	// States
	parameter STATE_INIT_SETUP 	= 3'h0,
				 STATE_INIT_WAIT		= 3'h1,
				 STATE_INIT_FINISH 	= 3'h2,
				 STATE_WAIT 			= 3'h3,
				 STATE_READ_WAIT_0	= 3'h4,
				 STATE_READ_WAIT_1	= 3'h5,
				 STATE_READ_FINISH	= 3'h6,
				 STATE_CONS_WAIT 		= 3'h7;
				 
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
			
			is_ready <= 0;
			need_car_or_cdr <= 0;
			should_fetch_data <= 0;
		end
		else begin
			case (state)
				// Initialize the free memory register
				// TODO - we want to clear the first memory address after we use it
				STATE_INIT_SETUP: begin
					state <= STATE_INIT_WAIT;
					mem_addr <= 0;
				end
				STATE_INIT_WAIT: begin
					state <= STATE_INIT_FINISH;
				end
				STATE_INIT_FINISH: begin
					state <= STATE_WAIT;
					free_mem <= mem_data_out;
				end
				// Wait for a command dispatch
				STATE_WAIT: begin
					if(execute) begin
						is_ready <= 0;
						// Dispatch according to the function
						case (func)
							GET_CAR: begin
								need_car_or_cdr <= 0;
								should_fetch_data <= 0;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
							GET_CDR: begin
								need_car_or_cdr <= 1;
								should_fetch_data <= 0;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
							GET_CONS: begin
								mem_data_in <= {4'h0, addr0, addr1};
								state <= STATE_CONS_WAIT;
							end
							GET_CONTENTS: begin
								should_fetch_data <= 1;
								mem_addr <= addr0;
								state <= STATE_READ_WAIT_0;
							end
						endcase
					end
					// Keep waiting
					else begin
						state <= STATE_WAIT;
						is_ready <= 1;
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
					is_ready <= 1;
					
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
				
				end
				default:;
			endcase
		end
	end

endmodule
