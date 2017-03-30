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
	
	// Memory unit interface
	reg [1:0] func;
	reg execute;
	reg [`memory_addr_width - 1:0] addr0, addr1;
	wire [`memory_addr_width - 1:0] addr_out;
	wire [`memory_data_width - 1:0] data_out;
	wire mem_ready;
	
	memory_unit mem(.func (func),
						 .execute (execute),
						 .addr0 (addr0),
						 .addr1 (addr1),
						 .addr_out (addr_out),
						 .data_out (data_out),
					    .is_ready (mem_ready),
						 .power (power),
					    .clk (clk),
					    .rst (rst));
						 
	reg [6:0] counter;
	reg [3:0] car_count;
	
	always@(posedge clk or negedge rst) begin
		if (!rst) begin
			counter <= 0;
			
			car_count <= 0;
			
			func <= 0;
			execute <= 0;
			addr0 <= 0;
			addr1 <= 0;
		end
		else if (power) begin
			counter <= counter + 1;
		
			if(counter >= 10) begin
				if(mem_ready) begin
					if(car_count == 0) begin
						addr0 <= 1;
						addr1 <= 2;
						func <= `GET_CONS;
						execute <= 1;
						
						car_count <= 1;
					end
					else if(car_count == 1) begin
						addr0 <= addr_out;
						addr1 <= 3;
						func <= `GET_CONS;
						execute <= 1;
						
						car_count <= 2;
					end
					else if(car_count == 2) begin
						addr0 <= addr_out;
						addr1 <= 4;
						func <= `GET_CONS;
						execute <= 1;
						
						car_count <= 3;
					end
					else if (car_count == 3) begin
						addr0 <= addr_out;
						func <= `GET_CAR;
						execute <= 1;
						
						car_count <= 4;
					end
					else if (car_count == 4) begin
						addr0 <= addr_out;
						func <= `GET_CAR;
						execute <= 1;
						
						car_count <= 5;
					end
					else if (car_count == 5) begin
						addr0 <= addr_out;
						func <= `GET_CONTENTS;
						execute <= 1;
						
						car_count <= 6;
					end
					else begin
						addr0 <= 0;
						func <= 0;
						execute <= 0;
					end
				end
				else begin
					addr0 <= 0;
					addr1 <= 0;
					func <= 0;
					execute <= 0;
				end
			end
		end
	end

endmodule
