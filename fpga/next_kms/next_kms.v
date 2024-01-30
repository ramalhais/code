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


reg [31:0] counter = 0;
reg [5:0] led_buf = 6'b110011;
reg [4:0] debug_test_pins;
assign led[5:0] = ~led_buf[5:0];

always@ (posedge clk) begin
    if (counter == 0) begin
        if (latest_keycode_valid) begin
            led_buf[5] <= latest_keycode_valid;
            led_buf[4:0] <= latest_keycode[4:0];
            // counter <= 32'd54000000;
            counter <= 32'd27000000;
            // counter <= 32'd06750000;
            // counter <= 1;
        end //else begin
        //     led_buf[5] <= latest_keycode_valid;
        //     led_buf[4:0] <= debug_test_pins;
        //     counter <= 32'd06750000;
        // end
    end else begin
        counter <= counter-1;
    end
end


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
    enable_next_keyboard,
    debug_test_pins
);

endmodule
