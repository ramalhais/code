`timescale 1ns/1ns

module test_DebugDataReceiver;

	reg clk = 0;
	reg data_start = 0;
	reg sin = 0;
	wire [39:0] data;
	wire out_valid;

	parameter CLOCK = 100;

	DebugDataReceiver receiver(
		clk,
		data_start,
		sin,
		data,
		out_valid
	);
	
	always #(CLOCK/2) clk = ~clk;

	initial begin
		#(CLOCK/4) sin = 0; // initial 
		data_start = 1;
		#CLOCK;
		sin = 1; // first bit
		data_start = 0;
				
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 1;
		#CLOCK sin = 1;
		#CLOCK sin = 1;
		
		#CLOCK sin = 0; 
		#CLOCK sin = 0;
		#CLOCK sin = 0;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		#CLOCK sin = 1; 
		#CLOCK sin = 0;
		#CLOCK sin = 0;
		#CLOCK sin = 1; // last bit
		
		
		#(CLOCK*5) sin = 0; // normal state
		data_start = 1;
		#CLOCK sin = 1; // first bit
		#(CLOCK) data_start = 0;
				
		#CLOCK sin = 0;
		#CLOCK sin = 1;
		#CLOCK sin = 0;
		
		$stop;
	end
	
endmodule
