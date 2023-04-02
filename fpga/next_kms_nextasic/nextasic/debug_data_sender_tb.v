`timescale 1ns/1ns

module test_DebugDataSender;

	reg in_latch = 0;
	reg in_clk = 0;
	reg out_clk = 0;
	reg [39:0] data;
	wire sout;
	wire state;
	wire int_state_1;
	wire int_state_2;

	parameter IN_CLOCK = 100;
	parameter OUT_CLOCK = IN_CLOCK*110; 	

	DebugDataSender sender(
		in_clk,
		in_latch,
		data,
		state,
		int_state_1,
		int_state_2,
		out_clk,
		sout
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	
	always #(OUT_CLOCK/2) out_clk = ~out_clk;
	
	initial begin
		data = 40'b1010100110011001100110011001100110010001;
		#IN_CLOCK in_latch = 1;
		#IN_CLOCK in_latch = 0;
		
		
		#(OUT_CLOCK*20);
		data = 40'b1010100110011001100110011001100110000001;
		#IN_CLOCK in_latch = 1;
		#IN_CLOCK in_latch = 0;
		#(OUT_CLOCK*20);
		
		#(OUT_CLOCK*7);
		
		data = 40'b1110100110011001100110011001100110010011;
		#IN_CLOCK in_latch = 1;
		#IN_CLOCK in_latch = 0;
		
		#(OUT_CLOCK*41);
		
		#(OUT_CLOCK*5);
		
		#OUT_CLOCK $stop;
	end
	
endmodule