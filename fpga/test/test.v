module test
(
    input clk,
    input btn1,
    input btn2,
    output [5:0] led
);

localparam WAIT_TIME = 13500000;
reg [5:0] ledCounter = 0;
reg [23:0] clockCounter = 0;
reg enabled = 1;

always @(posedge clk) begin
    if (enabled) begin
        clockCounter <= clockCounter + 1;
        if (clockCounter == WAIT_TIME) begin
            clockCounter <= 0;
            ledCounter <= ledCounter + 1;
        end
    end
end

assign led = ~ledCounter;

endmodule
