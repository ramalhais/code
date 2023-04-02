`timescale 1ns/1ns

module test_Keyboard;
	reg clk = 0;
	reg from_kb = 1;
	reg led_data_valid = 0;
	reg [1:0] led_data_in;
	wire to_kb, data_ready, is_mouse_data;
	wire [15:0] keyboard_data;
	
	parameter CLOCK = 200;
	
	parameter KEY_SIG_DELAY = 53000;
	
	localparam PACKET_READY = 21'b000000000110000000001;
	localparam PACKET_DATA =  21'b011011001010000000010;

	Keyboard keyboard(
		clk,
		led_data_valid,
		led_data_in,
		data_ready,
		is_mouse_data,
		keyboard_data,
		from_kb,
		to_kb
	);
	
	always #(CLOCK/2) clk = ~clk;
	
	task KeyboardRes(
		input [20:0] data
	);
		integer i;
		begin
			for(i = 0; i < 21; i = i + 1) begin
			      from_kb = data[20-i];
				  #(KEY_SIG_DELAY);
			end
			from_kb = 1;
		end
	endtask
	
	initial begin	
		#(CLOCK*5);
		
		@(negedge to_kb);
		#(KEY_SIG_DELAY*21); // reset packet
		
		#(KEY_SIG_DELAY*5);
		
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // kb query packet
		
		#(KEY_SIG_DELAY*3);
		
		// recv response
		KeyboardRes(PACKET_READY);
		
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // mouse query packet
		
		#(KEY_SIG_DELAY*3);
		
		// recv response
		KeyboardRes(PACKET_READY);
		
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // kb query packet
		
		#(KEY_SIG_DELAY*3);
		
		// recv response
		KeyboardRes(PACKET_DATA);
		
		// 
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // mouse query packet
		#(KEY_SIG_DELAY*3);
		KeyboardRes(PACKET_READY);
		
		//
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // kb query packet
		#(KEY_SIG_DELAY*3);
		KeyboardRes(PACKET_READY);
		
		//
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // mouse query packet
		#(KEY_SIG_DELAY*3);
		KeyboardRes(PACKET_READY);
		
		//
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // kb query packet
		#(KEY_SIG_DELAY*3);
		KeyboardRes(PACKET_READY);
		
		led_data_in = 2'b11;
		@(negedge clk) led_data_valid = 1;
		@(negedge clk) led_data_valid = 0;
		#(KEY_SIG_DELAY*40);
		
		//
		@(negedge to_kb);
		#(KEY_SIG_DELAY*8); // mouse query packet
		#(KEY_SIG_DELAY*3);
		KeyboardRes(PACKET_READY);
		
		
		#(KEY_SIG_DELAY*200);
		
		$stop;
	end
	
endmodule

//TODO: module test_Keyboard_no_connect;
