`timescale 1ns/1ns

module test_SenderCDC;

	reg in_clk = 0;
	reg out_clk = 0;
	reg [39:0] data;
	reg in_valid = 0;
	wire sout;

	
	parameter OUT_CLOCK = 100;
	parameter IN_CLOCK = OUT_CLOCK*100;

	SenderCDC sender(
		in_clk,
		data,
		in_valid,
		out_clk,
		sout
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;

	initial begin
		in_valid = 0;
		data = 40'b1101100110011001100110011001100110010001;
		#(IN_CLOCK);
		in_valid = 1;
		#(IN_CLOCK*2);
		in_valid = 0;
		
		#(OUT_CLOCK*48);
		#(OUT_CLOCK*20);
		
		data = 40'b1001100110011001100110011001100110010011;
		#(IN_CLOCK);
		in_valid = 1;
		#(IN_CLOCK*2);
		in_valid = 0;
		
		#(OUT_CLOCK*48);
		
		$stop;
	end
	
endmodule

