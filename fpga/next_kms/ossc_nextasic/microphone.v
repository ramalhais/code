`default_nettype none

module Microphone(
	input wire clk, // mon clk
	input wire record_start,
	input wire record_stop,

	input wire bclk, // i2s BCLK
	input wire audio_data_in, // i2s audio data
	input wire lrck, // i2s LRCK

	output reg [31:0] mic_data = 0,
	output wire mic_data_valid,
	input wire mic_data_retrieved,
	output wire [1:0] mic_debug
);

	reg record_active = 0;
	
	assign mic_debug[0] = record_active;

	wire [7:0] ulawout;
	reg [15:0] audio_data = 0; // 16 bit audio
	LIN2MLAW lin2ulaw(audio_data[15:15-12], ulawout);

	reg audio_data_available = 0;
	wire audio_data_available_;
	FF2SyncN audio_data_available__(audio_data_available, clk, audio_data_available_);
	
	reg audio_data_retrieved = 0;

	reg mic_data_filled = 0;
	assign mic_data_valid = record_active & mic_data_filled;
	reg [1:0] send_counter = 0; // 0 to 3
	assign mic_debug[1] = mic_data_valid;
	
	always@ (posedge clk) begin
		
		if (record_stop) begin
			record_active <= 0;
			// TODO: send last available mic data
		end else if (record_start) begin
			record_active <= 1;
		end
		
		if (record_active) begin
			if (mic_data_filled && mic_data_retrieved) begin
				mic_data_filled <= 0;
			end else if (!mic_data_filled) begin
				if (audio_data_available_ && !audio_data_retrieved) begin
					case (send_counter)
						2'd0: mic_data[31:24] <= ulawout;
						2'd1: mic_data[23:16] <= ulawout;
						2'd2: mic_data[15:8] <= ulawout;
						2'd3: begin
							mic_data[7:0] <= ulawout;
							mic_data_filled <= 1;
						end
					endcase
					send_counter <= send_counter + 1'd1;
					audio_data_retrieved <= 1;
				end else if (!audio_data_available_ && audio_data_retrieved) begin
					audio_data_retrieved <= 0;
				end
			end
		end
		
	end
	
	reg [4:0] recv_counter = 0; // 0 to 31

	reg lrck_p = 0;	
	always@ (posedge bclk) begin
		lrck_p <= lrck;
	
		if (lrck_p != lrck && lrck == 0) begin
			// Left channel
			recv_counter <= 0;
			audio_data <= 0;
		end else if (recv_counter < 16) begin
			recv_counter <= recv_counter + 1'd1;
		end
		
		if (recv_counter == 16) begin
			// audio data valid
			audio_data_available <= 1;
		end else begin
			audio_data_available <= 0;
			audio_data[15:0] <= {audio_data[14:0], audio_data_in};
		end
	end


endmodule
