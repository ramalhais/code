module SenderCDC(
	input wire in_clk,
	input wire [39:0] in_data,
	input wire in_valid,
	input wire out_clk,
	output wire sout // serial out
);

	localparam READY = 1'b0; // define state
	localparam SEND = 1'b1;

	reg [39:0] data_tmp;
	reg [40:0] data;
	reg [5:0] count = 0; // range 0 to 
	
	assign sout = data[40];
	reg state1 = READY;
	reg data_tmp_ready = 0;
	reg data_tmp_used = 0;
	reg data_tmp_used_ack = 0;

	always@ (posedge in_clk) begin
		if (in_valid && state1 == READY) begin
			data_tmp <= in_data;
			data_tmp_ready <= 1;
		end else begin
			data_tmp_ready <= 0;
		end
		data_tmp_used_ack <= data_tmp_used;
	end

	always@ (negedge out_clk) begin
		if (state1 == READY && !data_tmp_used && !data_tmp_used_ack) begin
			if (data_tmp_ready) begin
				data[40] <= 1;
				data[39:0] <= data_tmp;
				state1 <= SEND;
				count <= 0;
				data_tmp_used <= 1;
			end
		end
		if (data_tmp_used_ack)
			data_tmp_used <= 0;
		if (state1 == SEND) begin
			if (count == 41) begin
				state1 <= READY;
			end else begin
				data[0] <= 0;
				data[40:1] <= data[39:0];
				count <= count + 1'b1;
			end
		end
	end
	
endmodule
