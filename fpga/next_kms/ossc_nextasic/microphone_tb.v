`timescale 1ns/1ns

// TODO: update edge
module test_Microphone;
	reg mon_clk = 0;
	reg record_start = 0;
	reg record_stop = 0;
	reg bclk = 0;
	reg audio_data_in = 0;
	reg lrck = 0;
	
	wire [31:0] mic_data;
	wire mic_data_valid;
	
	reg mic_data_retrieved = 0;
	
	
	wire [1:0] mic_debug;
	
	parameter MON_CLOCK = 200;
	parameter AUDIO_CLOCK = MON_CLOCK*10; // BCLK

	// localparam PACKET_READY = 16'b5a;

	Microphone microphone(
		mon_clk,
		record_start,
		record_stop,
		bclk,
		audio_data_in,
		lrck,
		mic_data,
		mic_data_valid,
		mic_data_retrieved,
		mic_debug
	);
	
	always #(MON_CLOCK/2) mon_clk = ~mon_clk;
	
	always #(AUDIO_CLOCK/2) bclk = ~bclk;
	
	task I2SSend(
		input [15:0] lch,
		input [15:0] rch
	);
		integer i;
		begin
			lrck = 1;
			repeat(5) @(negedge bclk);
			lrck = 0;
			@(negedge bclk);
			for(i = 0; i < 16; i = i + 1) begin
				audio_data_in = lch[15-i];
				@(negedge bclk);
			end
			audio_data_in = 0;
			repeat(5) @(negedge bclk);
			lrck = 1;
			@(negedge bclk);
			for(i = 0; i < 16; i = i + 1) begin
				audio_data_in = rch[15-i];
				@(negedge bclk);
			end
			repeat(5) @(negedge bclk);
		end
	endtask
	
	initial begin
		forever begin
			@(posedge mic_data_valid);
			@(posedge mon_clk);
			@(posedge mon_clk);
			@(posedge mon_clk) mic_data_retrieved = 1;
			@(posedge mon_clk) mic_data_retrieved = 0;
		end
	end
	
	initial begin	
		#(MON_CLOCK*5);
		@(negedge mon_clk) record_start = 1;
		@(negedge mon_clk) record_start = 0;
		#(MON_CLOCK*5);
		
		I2SSend(16'h5a5a, 16'ha5a5);
		I2SSend(16'ha5a5, 16'h5a5a);
		I2SSend(16'h0001, 16'h0001);
		I2SSend(16'hffff, 16'hffff);
		
		I2SSend(16'h9696, 16'h9696);
		I2SSend(16'haaaa, 16'haaaa);
		I2SSend(16'haaaa, 16'haaaa);
		I2SSend(16'haaaa, 16'haaaa);
		
		I2SSend(16'haaaa, 16'haaaa);

		#(AUDIO_CLOCK*1000);
		$stop;
	end
	
endmodule

