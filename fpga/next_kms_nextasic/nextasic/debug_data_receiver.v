
module DebugDataReceiver(
	input wire debug_clk,
	input wire data_start,
	input wire sin,
	output reg [39:0] data,
	// output wire debug_test_out_1, // TODO: debug
	// output wire debug_test_out_2, // TODO: debug
	output reg out_valid_out = 0
);

	reg [5:0] count; // range 0 to 
	reg started = 0;
	reg started2 = 0;
	reg out_valid = 0;
	
	always@ (negedge debug_clk) begin
		out_valid_out <= out_valid;
	end
	
	always@ (posedge debug_clk) begin
		if (out_valid) out_valid <= 0;
		if (!started && data_start) begin
			count <= 0;
			started <= 1;
			started2 <= 1;
		end else begin
			if (started) begin
				if (count == 40) begin
					out_valid <= 1;
					started2 <= 0;
				end else begin
					data[0] <= sin;
					data[39:1] <= data[38:0];
					count <= count + 1'b1;
				end
				started <= started2;
			end
		end
	end


endmodule
