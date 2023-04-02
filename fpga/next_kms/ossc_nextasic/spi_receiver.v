`default_nettype none

module SPIReceiver(
    input wire clk, // may be system clock
    input wire [7:0] spi_data,
    input wire spi_data_valid_in,
    input wire spi_cs_N_in,
    output wire [23:0] spi_buf_out, // buf[0], [1], [2]
    output reg spi_buf_valid = 0
);

    reg spi_data_valid = 0;
    reg [1:0] counter = 0;
    reg [7:0] spi_buf[0:1];
    
    always @(negedge clk) begin
        spi_data_valid <= spi_data_valid_in;
        
        if (spi_buf_valid) begin
            spi_buf_valid <= 0;
        end else if (spi_data_valid_in && counter == 2'd2) begin
            // done, spi buffer is 3 bytes
            spi_buf_valid <= 1;
        end
    end

    assign spi_buf_out = {spi_buf[0], spi_buf[1], spi_data};

    wire spi_cs_n;
    FF2SyncN spi_cs__(spi_cs_N_in, clk, spi_cs_n);
    
    always @(posedge clk) begin
        if (spi_cs_n) begin
            counter <= 0;
        end else if (spi_data_valid && !spi_buf_valid) begin
            spi_buf[counter] = spi_data;
            counter = counter + 1'b1;
        end else if (spi_buf_valid)
            counter <= 0;
    end

endmodule
