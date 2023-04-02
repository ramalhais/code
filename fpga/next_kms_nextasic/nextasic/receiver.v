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
	
	always@ (negedge clk) begin
		if (count == 40)
			data_recv_valid <= 1;
		if (count == 41)
			data_recv_valid <= 0;
	end
	
	always@ (posedge clk) begin
		case (state)
			READY : if (si == 1) state <= READ; // start bit
			READ :
				case (count)
					41: begin
						state <= READY;
						count <= 0;
					end
					40: begin
						count <= count + 1'b1;
					end
					default: begin
						data[39:0] <= {data[38:0], si};
						count <= count + 1'b1;
					end
				endcase
		endcase
	end


endmodule
