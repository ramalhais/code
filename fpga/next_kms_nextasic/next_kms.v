module next_kms
(
    input clk,
    input btn1,
    input btn2,
    output [5:0] led,

    // for NeXT computer
    input wire mon_clk, // NeXT pin 3 (mono/color)
    input wire to_mon, // NeXT pin 4 (mono/color)
    output wire from_mon, // NeXT pin 5 (mono/color)

    // for Keyboard
	input wire from_kb,
	output wire to_kb,
);

// (* keep="soft" *) // PR: maybe not needed?
wire [23:0] unconnected;

assign led = unconnected[16:11];
// assign led = ~unconnected[21:16];

nextasic next_asic(
	// for NeXT computer
	mon_clk,
	to_mon,
	from_mon,
	
	// for Keyboard
	from_kb,
	to_kb,
	
	// for DAC
	unconnected[0],//input wire mclk,
	unconnected[1],//output wire mclk_out,
	unconnected[2],//output wire bclk,
	unconnected[3],//output wire lrck,
	unconnected[4],//output wire audio_data,
	
	//
	unconnected[5],//input wire dummy_clk,
	
	unconnected[6],//input wire debug_clk,
	unconnected[7],//output wire debug_sout,
	unconnected[8],//output wire debug_sig_on, // state
	unconnected[9],//input wire debug_sin,
	unconnected[10],//input wire debug_sin_start,

	unconnected[21:11],//output wire [9:0] debug_test_pins_out,
	unconnected[22],//input wire debug_sw_0,
	unconnected[23],//input wire debug_sw_1
);

endmodule
