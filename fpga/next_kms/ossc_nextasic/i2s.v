`default_nettype none

module I2SSender(
	input wire in_clk,
	input wire in_valid,
	input wire [31:0] in_data,
	input wire audio_start_in,
	input wire audio_end_in,
	input wire audio_22kz_in, // 1 is 22khz, 0 is 44khz
	output wire audio_req_mode_out, // able to request sound samples to NeXT hardware
	output wire audio_req_underrun_out,
	output reg audio_req_tick = 0,
	//input wire i2s_clk, // 22.5792Mhz(or 11.2896Mhz)
	input wire bck, // 2.8224Mhz, 64fs
	output wire lrck, // 1fs=44.1khz	
	output reg sout = 0 // i2s data, serial out
);

	localparam IN_SAMPLES_L = 1'b0;
	localparam IN_SAMPLES_R = 1'b1;
	localparam REQ_OUT_DELAY = 4;

	reg state = IN_SAMPLES_R;
	reg data1_filled = 0;
	wire data1_filled_;
	FF2SyncP data1_filled__(data1_filled, bck, data1_filled_);
	reg data1_retrieved = 0;
	wire data1_retrieved_;
	FF2SyncP data1_retrieved__(data1_retrieved, in_clk, data1_retrieved_);
	reg data2_valid = 0; // data2 has complete sample data is not during shifting
	reg can_serial_out = 0;
	reg [31:0] data1;
	reg [31:0] data2;
	reg [5:0] counter = 0; // 0 to 63
	reg [1:0] send_count = 0;
	reg [5:0] req_delay = 0;
	reg [2:0] underrun_count = 0;
	
	reg audio_req_interval = 0; // for 22khz, // TODO: remove
	
	reg audio_req = 0; // bck domain
	wire audio_req_;
	FF2SyncN audio_req__(audio_req, in_clk, audio_req_);
	reg audio_req_ack = 0; // in_clk domain
	wire audio_req_ack_;
	FF2SyncP audio_req_ack__(audio_req_ack, bck, audio_req_ack_);
	reg audio_start = 0; // in_clk domain
	wire audio_start_;
	FF2SyncP audio_start__(audio_start, bck, audio_start_);	
	
	reg audio_on_req_mode = 0; // bck domain, sync to req_delay
	wire audio_on_req_mode_;
	assign audio_req_mode_out = audio_start & audio_on_req_mode_;
	FF2SyncP audio_on_req_mode__(audio_on_req_mode, in_clk, audio_on_req_mode_);
	reg on_req_mode = 0;
	
	reg audio_22k = 0; // in_clk domain
	wire audio_22k_;
	FF2SyncP audio_22k__(audio_22k, bck, audio_22k_);
	
	wire audio_req_underrun;
	assign audio_req_underrun = underrun_count >= (audio_22k_ ? 3'd5: 3'd3) ? 1'b1 : 0; // bck domain
	wire audio_req_underrun_;
	FF2SyncP audio_req_underrun__(audio_req_underrun, in_clk, audio_req_underrun_);
	assign audio_req_underrun_out = audio_start & audio_req_underrun_;

	assign lrck = state;
	
	always@ (negedge bck) begin
		// request
		if (audio_req_ack_)
			audio_req <= 0;
			
		if (req_delay == (audio_22k_ ? 6'd21 : 6'd22)) begin // TODO: timing, 5us?
			req_delay <= 0;
			// audio_req <= 1;
			if (audio_req_interval && audio_22k_)
				audio_req_interval <= 0;
			else begin
				audio_req_interval <= 1;
				audio_req <= 1;
			end
			audio_on_req_mode <= on_req_mode;
		end else begin
			req_delay <= req_delay + 1'b1;
			if (!audio_start_)
				audio_req_interval <= 0; 
		end
		
		// audio data
		if (counter == 31) begin
			if (data2_valid || data1_filled_ || !audio_start_)
				underrun_count <= 0;
			else if (underrun_count < 3'd7 && audio_start_ && on_req_mode)
				underrun_count <= underrun_count + 1'b1;
			
			case (state)
				IN_SAMPLES_R: begin
					state <= IN_SAMPLES_L;
					can_serial_out <= data2_valid | (send_count > 0);
				end
				IN_SAMPLES_L: begin
					state <= IN_SAMPLES_R;
					req_delay <= 0;
					if (send_count && can_serial_out) begin
						send_count <= send_count - 1'b1; // send same data in data2 again
						if (!data1_filled_) begin
							on_req_mode <= audio_start_;
						end else
							on_req_mode <= 0;
					end else
						on_req_mode <= audio_start_;
				end
			endcase
			counter <= 0;
		end else begin
			counter <= counter + 1'b1;
		end
		
		if (!data1_filled_)
			data1_retrieved <= 0;
		
		if (state == IN_SAMPLES_L)
			req_delay <= 0;
		
		if (can_serial_out && counter <= 15) begin
			sout <= data2[31];
			data2[31:1] <= data2[30:0];
			data2[0] <= data2[31];
			data2_valid <= 0; // data2 is partial data, is shifing...
		end else begin
			sout <= 0;
			if (state == IN_SAMPLES_R && send_count == 0 && !data2_valid && data1_filled_) begin
				// get next data
				data2 <= data1;
				data2_valid <= 1;
				data1_retrieved <= 1;
				//
				if (audio_22k_)
					on_req_mode <= 0;
				else
					on_req_mode <= audio_start_;
				send_count <= audio_22k_ ? 2'd2 : 0; // 2 = 22khz, 0 = 44khz
			end
		end
	end
	
	always@ (posedge in_clk) begin
		if (!audio_req_ack && audio_req_) begin
			audio_req_tick <= 1;
			audio_req_ack <= 1;
		end else if (audio_req_tick)
			audio_req_tick <= 0;
			
		if (!audio_req_)
			audio_req_ack <= 0;
	end
	
	always@ (posedge in_clk) begin
		// request
		if (audio_end_in) begin
			audio_start <= 0;
			audio_22k <= audio_22kz_in;
		end else if (audio_start_in) begin
			audio_start <= 1;
			audio_22k <= audio_22kz_in;
		end
		
		// audio data
		if (data1_retrieved_)
			data1_filled <= 0;
						
		if (in_valid && !data1_filled) begin
			data1 <= in_data;
			data1_filled <= 1;
		end
	end



endmodule
