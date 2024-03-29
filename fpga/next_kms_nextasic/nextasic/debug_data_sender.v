module DebugDataSender(
	input wire in_clk,
	input wire in_data_valid, // data valid
	input wire [39:0] data,
	output wire state_out_,
	output wire debug_test_out_1, // TODO: debug
	output wire debug_test_out_2, // TODO: debug
	input wire out_clk,
	output wire sout
);

	localparam EMPTY = 1'b0; // define state
	localparam STORED = 1'b1;

	reg out_state = EMPTY;
	wire out_state_;
	FF2SyncP out_state__(out_state, out_clk, out_state_);
	reg [39:0] stored; // stored data
	reg [39:0] tmp;
	reg in_state = EMPTY;
	wire in_state_;
	FF2SyncP in_state__(in_state, out_clk, in_state_);
	reg [5:0] count = 0; // range 0 to ...
	reg in_state_ack = 0;
	wire in_state_ack_;
	FF2SyncP in_state_ack__(in_state_ack, in_clk, in_state_ack_);
	
	assign sout = stored[39];
	
	assign debug_test_out_1 = in_state;
	assign debug_test_out_2 = out_state;
	
	reg state_out = 0;
	assign state_out_ = state_out;
	


	always@ (negedge out_clk) begin
		if (in_state_ == EMPTY) begin
			in_state_ack <= 0;
		end
		if (in_state_ == STORED && out_state == EMPTY) begin
			in_state_ack <= 1;
			out_state <= STORED;
			count <= 1;
			stored <= tmp;
			state_out <= 1;
		end
		if (out_state == STORED) begin
			state_out <= 0;
			if (count == 45) begin
				out_state <= EMPTY;
			end else begin
				stored[39:1] <= stored[38:0];
				count <= count + 1'b1;
			end
		end
	end
	
	always@ (posedge in_clk) begin
		if (in_state_ack_) begin
			in_state <= EMPTY;
		end
		if (in_data_valid && in_state == EMPTY) begin
			tmp <= data;
			in_state <= STORED;
		end
	end
	


endmodule
