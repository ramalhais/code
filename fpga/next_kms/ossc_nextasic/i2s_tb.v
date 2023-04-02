`timescale 1ns/1ns



module test_I2SSender;

	reg in_clk = 0;
	reg out_clk = 0; // bck
	reg [31:0] data;
	reg in_valid = 0;
	reg audio_start = 0;
	reg audio_end = 0;
	wire lrck;
	wire sout;
	wire audio_req_tick;
	wire audio_req_mode_out;
	wire audio_req_underrun;

	
	parameter OUT_CLOCK = (355); // 2.82mhz
	parameter IN_CLOCK = 200;

	I2SSender sender(
		in_clk,
		in_valid,
		data,
		audio_start,
		audio_end,
		0,
		audio_req_mode_out,
		audio_req_underrun,
		audio_req_tick,
		out_clk, // bck
		lrck,
		sout
	);
	
	integer i;
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;

	initial begin
		in_valid = 0;
		data = 32'b11011001100110011001100110010001;
		
		#(OUT_CLOCK*35*4);
		
		@(negedge in_clk);
		audio_start = 1;
		@(negedge in_clk);
		audio_start = 0;
		
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*20);
		data = 32'b10011001100110011001100110010011;
		@(negedge in_clk) in_valid = 1;
		@(negedge in_clk) in_valid = 0;
		
		// simulate underrun
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*300);
		data = 32'b10011001100110011001100110010011;
		// @(negedge in_clk) in_valid = 1;
		// @(negedge in_clk) in_valid = 0;
		
		for (i = 0; i < 5; i ++) begin
			@(negedge audio_req_mode_out & audio_req_tick);
			#(OUT_CLOCK*20);
			data = 32'b10011001100110011001100110000011;
			@(negedge in_clk) in_valid = 1;
			@(negedge in_clk) in_valid = 0;
		end
		
		// simulate underrun
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*200);
		@(negedge in_clk) audio_end = 1;
		@(negedge in_clk) audio_end = 0;
		
		// #(OUT_CLOCK*64*1);
// 		@(negedge in_clk) audio_end = 1;
// 		@(negedge in_clk) audio_end = 0;
		
		#(OUT_CLOCK*64*11);
		
		
		$stop;
	end
	
endmodule


module test_I2SSender_22khz;

	reg in_clk = 0;
	reg out_clk = 0; // bck
	reg [31:0] data;
	reg in_valid = 0;
	reg audio_start = 0;
	reg audio_end = 0;
	wire lrck;
	wire sout;
	wire audio_req_tick;
	wire audio_req_mode_out;
	wire audio_req_underrun;

	
	parameter OUT_CLOCK = (355); // 2.82mhz
	parameter IN_CLOCK = 200;

	I2SSender sender(
		in_clk,
		in_valid,
		data,
		audio_start,
		audio_end,
		1, // 22khz
		audio_req_mode_out,
		audio_req_underrun,
		audio_req_tick,
		out_clk, // bck
		lrck,
		sout
	);
	
	integer i;
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;

	initial begin
		in_valid = 0;		
		#(OUT_CLOCK*35*4);
		
		@(negedge in_clk);
		audio_start = 1;
		@(negedge in_clk);
		audio_start = 0;
		
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*20);
		data = 32'b11011001100110011001100110010001;
		@(negedge in_clk) in_valid = 1;
		@(negedge in_clk) in_valid = 0;
		
		// simulate underrun
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*300);
		data = 32'b10011001100110011001100110010011;
		// @(negedge in_clk) in_valid = 1;
		// @(negedge in_clk) in_valid = 0;
		
		for (i = 0; i < 5; i ++) begin
			@(negedge audio_req_mode_out & audio_req_tick);
			#(OUT_CLOCK*20);
			data = 32'b10011001100110011001100110000011;
			@(negedge in_clk) in_valid = 1;
			@(negedge in_clk) in_valid = 0;
		end
		
		
		// simulate underrun
		@(negedge audio_req_mode_out & audio_req_tick);
		#(OUT_CLOCK*300);
		data = 32'b10011001100110011001100110010011;
		// @(negedge in_clk) in_valid = 1;
		// @(negedge in_clk) in_valid = 0;
		
		#(OUT_CLOCK*300);
		@(negedge in_clk) audio_end = 1;
		@(negedge in_clk) audio_end = 0;
		
		#(OUT_CLOCK*64*11);
		
		$stop;
	end
	
endmodule
