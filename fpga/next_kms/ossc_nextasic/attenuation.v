`default_nettype none

module Attenuation(
	input wire clk,
	input wire attenuation_data_valid,
	input wire [7:0] data_in, // attenuation_data from NeXT hardware
	output reg is_muted = 1,
	output reg [5:0] lch_db = 0, // 43(-86dB) to 0(0dB)
	output reg [5:0] rch_db = 0,
	output wire db_val_valid
	//output wire [7:0] debug_out
	//output wire value_updated
);

	localparam VAL1_0 = 8'h?2;
	localparam VAL1_1 = 8'h?6;
	//localparam VAL0_0 = 8'h00;
	localparam VAL0_1 = 8'h?4;
	
	// state definition
	localparam S_VAL1_0 = 3'd1;
	localparam S_VAL1_1 = 3'd2;
	localparam S_VAL0_0_OR_S_0 = 3'd0;
	localparam S_VAL0_1 = 3'd3;
	localparam S_1 = 3'd4;
	localparam S_INVALID = 3'd7;
	
	localparam CMD_INVALID = 2'b00;
	localparam CMD_L_CH = 2'b01;
	localparam CMD_R_CH = 2'b10;
	localparam CMD_BOTH_CH = CMD_L_CH | CMD_R_CH;
	
	reg [2:0] state = S_VAL0_0_OR_S_0; 
	reg [10:0] buff;
	reg [3:0] count = 0;
	reg initialized = 0;
	
	wire [1:0] cmd;
	assign cmd = buff[7:6];
	wire [5:0] att_data;
	assign att_data = buff[5:0];
	
	reg db_val_valid_l = 0;
	reg db_val_valid_r = 0;
	
	assign db_val_valid = db_val_valid_l & db_val_valid_r;
	
	// assign debug_out = buff[7:0];
	
	reg [2:0] valid_state;
	always@ (*) begin
		casex (data_in)
			8'b000?0000: // ? is mute flag
				valid_state = S_VAL0_0_OR_S_0;
			8'h?1:
				valid_state = S_1;
			VAL1_0:
				valid_state = S_VAL1_0;
			VAL1_1:
				valid_state = S_VAL1_1;
			VAL0_1:
				valid_state = S_VAL0_1;
			default:
				valid_state = S_INVALID;
		endcase
	end
		
	wire is_eof_packet;
	assign is_eof_packet = (count == 4'd11);

	always@ (posedge clk) begin
		if (attenuation_data_valid) begin
			is_muted <= data_in[4]; // here is mute flag
			case (valid_state)
				S_VAL0_0_OR_S_0: begin
					if (!is_eof_packet && state != S_VAL0_1 && state != S_VAL1_1) begin
						// reset current state
						buff <= 0;
						count <= 0;
						initialized <= 1;
					end
				end
				S_VAL1_1: if (initialized && state == S_VAL1_0) begin
					buff[0] <= 1'b1;
					buff[10:1] <= buff[9:0];
					count <= count + 1'b1;
				end
				S_VAL0_1: if (initialized && state == S_VAL0_0_OR_S_0) begin
					buff[0] <= 1'b0;
					buff[10:1] <= buff[9:0];
					count <= count + 1'b1;
				end
				// buff[10:8] is header will be 111
				S_1: if (initialized && buff[10:8] == 3'b111 && state == S_VAL0_0_OR_S_0 && is_eof_packet) begin
					case (cmd)
						CMD_L_CH: begin
							lch_db <= att_data;
							db_val_valid_l <= 1;
						end
						CMD_R_CH: begin
							rch_db <= att_data;
							db_val_valid_r <= 1;
						end
						CMD_BOTH_CH: begin
							lch_db <= att_data;
							rch_db <= att_data;
							db_val_valid_l <= 1;
							db_val_valid_r <= 1;
						end
						CMD_INVALID: begin						
						end
					endcase
					initialized <= 0;
					count <= 0;
				end
			endcase	
			state <= valid_state;
		end
	end

endmodule
