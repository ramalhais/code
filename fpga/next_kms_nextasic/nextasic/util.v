`timescale 1ns/1ns
`default_nettype none

module FF2SyncP(
	input wire in,
	input wire out_clk,
	output wire out_data
);

	reg f1, f2;
	
	assign out_data = f2;
	
	always@ (posedge out_clk) begin
		f1 <= in;
		f2 <= f1;
	end
endmodule

module FF2SyncN(
	input wire in,
	input wire out_clk,
	output wire out_data
);

	reg f1, f2;
	
	assign out_data = f2;
	
	always@ (negedge out_clk) begin
		f1 <= in;
		f2 <= f1;
	end
endmodule

module Delay #(
	parameter DELAY = 10,
	parameter W = 4
	) (
	input wire clk,
	input wire in_data,
	input wire reset,
	output reg out_data = 0
);
	reg [W-1:0] counter;
	reg running = 0;
	
	always@ (negedge clk) begin
		if (counter == DELAY)
			out_data <= 1;
		else if (out_data) out_data <= 0;
	end
	
	always@ (posedge clk) begin
		if (reset) 
			running <= 0;
		else begin
			if (running) begin
				if (counter == (DELAY+1)) begin
					running <= 0;
				end else begin
					counter <= counter + 1'b1;
				end
			end else begin
				if (in_data && !running) begin
					counter <= 0;
					running <= 1;
				end
			end
		end
	end
endmodule

module Divider#(
	parameter DIVISOR = 4'd8,
	parameter W = 3
	) (
	input wire clk,
	output wire out
);
	reg[W-1:0] counter = 0;
	
	assign out = counter[2];
	
	always @(posedge clk) begin
		if(counter >= (DIVISOR-1))
			counter <= 0;
		else begin
			counter <= counter + 1'b1;
		end
	end
endmodule
