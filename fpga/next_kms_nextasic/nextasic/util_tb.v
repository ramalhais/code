module test_Delay;
	reg clk = 0;
	reg in_data = 0;
	wire out;
	
	parameter CLOCK = 200;

	Delay delay(
		clk,
		in_data,
		0,
		out
	);
	
	always #(CLOCK/2) clk = ~clk;
	
	initial begin	
		#(CLOCK*5);
		@(negedge clk);
		in_data = 1;
		@(negedge clk);
		in_data = 0;
		#(CLOCK*50) $stop;
	end
	
endmodule

module test_Divider;

	reg clk = 0;
	wire out;
	
	parameter CLOCK = 100;

	Divider div(
		clk,
		out
	);
	
	always #(CLOCK/2) clk = ~clk;
	
	
	initial begin		
		#(CLOCK*50) $stop;
	end
	
endmodule
