`timescale 1ns/1ns
`default_nettype none

module DataSync #(
	parameter W = 4
	) (
	input wire in_clk,
	input wire [W-1:0] in_data,
	input wire in_data_valid,

	input wire out_clk,
	output reg [W-1:0] out_data,
	output reg out_data_valid = 0,
	input wire out_data_retrieved
);

	reg [W-1:0] tmp;
	reg has_data = 0;
	wire has_data_;
	FF2SyncN has_data__(has_data, out_clk, has_data_);
	reg data_retrived = 0;
	wire data_retrived_;
	FF2SyncN data_retrived__(data_retrived, in_clk, data_retrived_);

	always@ (posedge in_clk) begin
		if (!has_data && in_data_valid) begin
			tmp <= in_data;
			has_data <= 1; // TODO: data loss
		end else if (data_retrived_) begin
			has_data <= 0;
		end
	end
	
	always@ (posedge out_clk) begin
		if (has_data_ && !out_data_valid && !data_retrived) begin
			out_data <= tmp;
			out_data_valid <= 1;
		end else if (out_data_valid && !data_retrived && out_data_retrieved) begin
			out_data_valid <= 0;
			data_retrived <= 1;
		end else if (!has_data_ && data_retrived) begin
			data_retrived <= 0;
		end
	end

endmodule
