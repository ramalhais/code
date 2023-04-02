`timescale 1ns/1ns

module test_Attenuation;

	reg clk = 0;
	reg attenuation_data_valid = 0;
	reg [7:0] data;
	wire is_muted;
	wire [5:0] lch_db;
	wire [5:0] rch_db;
	wire db_val_valid;

	parameter CLOCK = 100;

	Attenuation att(
		clk,
		attenuation_data_valid,
		data,
		is_muted,
		lch_db,
		rch_db,
		db_val_valid
	);
	
	always #(CLOCK/2) clk = ~clk;

	task PacketSend(
		input [7:0] in_data
	);
		begin
			data = in_data;
			@(negedge clk);
			attenuation_data_valid = 1;
			@(negedge clk);
			attenuation_data_valid = 0;
			#(CLOCK*3);
		end
	endtask
	
	initial begin
		#(CLOCK*5);
		PacketSend(8'h17); // mute packet
		
		
		// test for att L ch
		PacketSend(8'h00);
		
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, header
		
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, cmd
		
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, att data
		
		PacketSend(8'h00);
		PacketSend(8'h01); // commit 
		PacketSend(8'h00);
		
		// test with invalid packet
		PacketSend(8'h00);
		PacketSend(8'h00);
		
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, header
		
		PacketSend(8'h04); // invalid packet
		PacketSend(8'h20);
		
		// test for att both ch
		PacketSend(8'h00);
		
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, header
		
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, cmd
		
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h00);
		PacketSend(8'h04); // 0
		PacketSend(8'h02);
		PacketSend(8'h06); // 1
		PacketSend(8'h02);
		PacketSend(8'h06); // 1, att data
		
		PacketSend(8'h00);
		PacketSend(8'h01); // commit 
		PacketSend(8'h00);
		
		$stop;
	end
	
endmodule
