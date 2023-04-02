`timescale 1ns/1ns

module test_Receiver;

	reg clk = 0;
	reg sin;
	wire [39:0] data;

	parameter CLOCK = 100;

	Receiver receiver(
		clk,
		sin,
		data
	);
	
	always #(CLOCK/2) clk = ~clk;

	initial begin
		#(CLOCK/4) sin = 0; // initial 
		
		#CLOCK sin = 1; // first, will be dropped
		
		#CLOCK sin = 1; // MSB
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
		#CLOCK sin = 1; // last bit, LSB
		
		
		#CLOCK sin = 0; // normal state
		#CLOCK;
		#CLOCK;
		#CLOCK;
		
		
		#CLOCK sin = 1; // first, will be dropped
		
		#CLOCK sin = 1;
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
		
		#CLOCK sin = 0;
		
		#(CLOCK*5);
		
		
		#CLOCK $stop;
		
		// TODO: check output
	
	end

endmodule
