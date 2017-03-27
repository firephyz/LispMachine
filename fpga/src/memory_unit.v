module memory_unit(get_car, get_cdr, data_in, data_out, is_ready, clk, rst);

	input clk, rst;

	input get_car, get_cdr;
	input [`memory_data_width - 1:0] data_in, data_out;
	output reg is_ready;
	
	reg has_init;
	
	always@(posedge clk or negedge rst) begin
		if(!rst) begin
			has_init <= 0;
			is_ready <= 0;
		end
		else begin
		
		end
	end

endmodule
