`timescale 1ns/1ns

module test_Sender;

	reg clk = 0;
	reg [39:0] data;
	reg data_valid = 0;
	reg audio_sample_request_tick = 0;
	reg audio_sample_request_mode = 0;
	reg audio_sample_request_underrun = 0;
	wire sout;
	wire data_loss;
	wire data_retrieved;
	
	parameter CLOCK = 100;
	parameter AUDIO_REQ_INTERVAL = 114;

	Sender sender(
		clk,
		data,
		data_valid,
		audio_sample_request_mode,
		audio_sample_request_underrun,
		audio_sample_request_tick,
		sout,
		data_loss,
		data_retrieved
	);
	
	always #(CLOCK/2) clk = ~clk;
	always #(CLOCK*AUDIO_REQ_INTERVAL) begin
		@(posedge clk);
		audio_sample_request_tick = 1;
		@(posedge clk);
		audio_sample_request_tick = 0;
	end

	task sendData;
		begin
			data = 40'b1101100110011001100110011001100110010001;
			@(posedge clk);
			data_valid = 1;
			@(posedge clk);
			data_valid = 0;
			#(CLOCK*20);
		
			data = 40'b1101100110011001100110011001100110010011;
			@(posedge clk);
			data_valid = 1;
			@(posedge clk);
			data_valid = 0;
			#(CLOCK*10);
		
			data = 40'b1101100110011001100110011001100110010111;
			@(posedge clk);
			data_valid = 1;
			@(posedge clk);
			data_valid = 0;
		end
	endtask
	
	initial begin
		// regular send
		sendData();
		#(CLOCK*AUDIO_REQ_INTERVAL +40);
		
		// with audio sample request
		audio_sample_request_mode = 1;
		
		#(CLOCK*60);
		sendData();
		#(CLOCK*AUDIO_REQ_INTERVAL*2);
		audio_sample_request_mode = 0;
		#(CLOCK*AUDIO_REQ_INTERVAL*2);
		
		$stop;
	end
	
endmodule
