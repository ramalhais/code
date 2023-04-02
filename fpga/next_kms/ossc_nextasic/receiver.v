`default_nettype none

module Receiver(
	input wire clk,
	input wire si, // serial in
	output reg [39:0] data = 0, // 10bytes(40bits)
	output reg data_recv_valid = 0
);
	
	localparam READY = 1'b0; // define state
	localparam READ = 1'b1;

	reg state = READY;
	reg [5:0] count = 0; // range 0 to 40
	reg is_all_1 = 0;
	
	always@ (negedge clk) begin
		if (count == 40)
			data_recv_valid <= 1;
		if (count == 41)
			data_recv_valid <= 0;
	end
	
	reg reset_req = 0; // TODO: use this reset line for global reset
	
	always@ (posedge clk) begin
		if (reset_req) begin
			state <= READY;
			count <= 0;
			if (!si)
				reset_req <= 0;
		end else
			case (state)
				READY : if (si == 1) begin
					state <= READ; // start bit
				end
				READ :
					case (count)
						41: begin
							state <= READY;
							count <= 0;
							if (!si)
								is_all_1 <= 0;
							else if (is_all_1)
								reset_req <= 1;
						end
						40: begin
							count <= count + 1'b1;
						end
						default: begin
							if (si && count == 0)
								is_all_1 <= 1;
							else if (!si)
								is_all_1 <= 0;
							data[39:0] <= {data[38:0], si};
							count <= count + 1'b1;
						end
					endcase
			endcase
	end


endmodule
