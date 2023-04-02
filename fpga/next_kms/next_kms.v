module next_kms
(
    input clk,
    input btn1,
    input btn2,
    output [5:0] led,

    // for NeXT computer
    input mon_clk, // NeXT pin 3 (mono/color)
    input to_mon, // NeXT pin 4 (mono/color)
    output from_mon, // NeXT pin 5 (mono/color)

    // for Keyboard
	input from_kb,
	output to_kb
);

assign led = ~{latest_keycode_valid,is_muted,volume_db_valid,latest_keycode[2:0]};

// Unused
wire hw_reset_n;
wire spdif_led0;

wire mic_bclk;
wire mic_lrck;
wire mic_data;

wire audio_mclk;
wire audio_bclk;
wire audio_lrck;
wire audio_data;

wire [15:0] latest_keycode;
wire latest_keycode_valid;
wire is_muted;
wire [11:0] volume_db;
wire volume_db_valid;
reg enable_next_keyboard = 1;

NextSoundBox nextsb(
    clk, // 27MHz
    hw_reset_n,

    from_kb,
    to_kb,

    mon_clk,
    to_mon,
    from_mon,

    spdif_led0,

    mc_sck,
    mc_mosi,
    mc_ss,
    mc_miso,

    mic_bclk,
    mic_lrck,
    mic_data,

    audio_mclk,
    audio_bclk,
    audio_lrck,
    audio_data,

    latest_keycode,
    latest_keycode_valid,
    is_muted,
    volume_db, 
    volume_db_valid,
    enable_next_keyboard
);

endmodule
