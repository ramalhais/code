`default_nettype none

module Sender(
	input wire clk,
	input wire [39:0] in_data,
	input wire in_data_valid,
	input wire audio_sample_request_mode,
	input wire audio_sample_request_underrun,
	input wire audio_sample_request_tick,
	output wire sout, // serial out
	output reg data_loss = 0,
	output wire data_retrieved,
	output [2:0] debug
);

	assign debug[2:0] = {data_retrieved,has_buffer_data,state};

	localparam READY = 1'b0; // define state
	localparam SEND = 1'b1;

	reg [39:0] buffer;
	reg has_buffer_data = 0;

	reg [40:0] data = 0;
	reg [6:0] count = 0; // range 0 to

	assign sout = data[40];

	reg state = READY;

	wire packet_send_end;
	assign packet_send_end = count == (41+3);

	reg data_retrieved_reg = 0;
	assign data_retrieved = data_retrieved_reg | (in_data_valid & !has_buffer_data);

	always@ (posedge clk) begin
		if (in_data_valid & !has_buffer_data)
			data_retrieved_reg <= 1;
		else
			data_retrieved_reg <= 0;
	end

	always@ (posedge clk) begin
		case (state)
			READY: begin
				if (audio_sample_request_tick) begin
					if (audio_sample_request_underrun) begin
						data[40] <= 1;
						data[39:0] <= 40'h0f00000000; // audio underrun detect packet
					end else if (audio_sample_request_mode) begin
						data[40] <= 1;
						data[39:0] <= 40'h0700000000; // audio sample request packet
					end
				end else begin
						data[40] <= 0;
						data[39:0] <= 0;
				end
				state <= SEND;
				count <= 0;

				if (in_data_valid) begin
					if (has_buffer_data)
						data_loss <= 1;
					else begin
						has_buffer_data <= 1;
						buffer <= in_data;
					end
				end
			end
			SEND: begin
				if (packet_send_end) begin
					data_loss <= has_buffer_data & in_data_valid;
					if (has_buffer_data) begin
						data[40] <= 1;
						data[39:0] <= buffer;
						has_buffer_data <= 0;
					end else if (in_data_valid) begin
						buffer <= in_data;
						has_buffer_data <= 1;
					end
					count <= count + 1'b1;
				end else if (count == (41+3+41+1)) begin
					state <= READY;
					data_loss <= 0;
				end else begin
					data[0] <= 0;
					data[40:1] <= data[39:0];
					count <= count + 1'b1;
				end
				if (!packet_send_end) begin
					if (in_data_valid) begin
						if (has_buffer_data)
							data_loss <= 1;
						else begin
							has_buffer_data <= 1;
							buffer <= in_data;
						end
					end
				end
			end
		endcase
	end

endmodule
