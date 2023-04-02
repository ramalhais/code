`timescale 1ns/1ns


module test_SPIReceiver;
    reg clk = 0;
    
    reg spi_clk = 0;
    reg spi_mosi = 0;
    reg spi_cs_N = 1;
    wire spi_miso;

    wire [7:0] spi_data;
    wire spi_data_valid;
    
    wire [23:0] spi_buf;
    wire spi_buf_valid;
    
    reg reset_n = 0;
    SPI_Slave #(.SPI_MODE(1)) spi_slave(
        reset_n,
        clk,
        spi_data_valid,
        spi_data,
        1'b1,
        8'h00,
        spi_clk,
        spi_miso,
        spi_mosi,
        spi_cs_N
    );
    
    SPIReceiver spi_recv(
        clk,
        spi_data,
        spi_data_valid,
        spi_cs_N,
        spi_buf,
        spi_buf_valid
    );

    parameter CLOCK = 100;
    parameter SPI_CLOCK = 300;

    always #(CLOCK/2) clk = ~clk;
    
    task SPISendSingle(
        input [7:0] data
    );
        integer i;
    begin
        spi_cs_N = 0;
        #(SPI_CLOCK);
            for(i = 0; i < 8; i = i + 1) begin
                spi_mosi = data[7-i];
                spi_clk = 1;
                #(SPI_CLOCK);
                spi_clk = 0;
                #(SPI_CLOCK);
            end
        spi_cs_N = 1;
        spi_mosi = 0;
    end
    endtask
    
    localparam SPI_BYTES = 3;
    
    task SPISendMulti(
        input [8*SPI_BYTES-1:0] data
    );
        integer s, b;
    begin
        spi_cs_N = 0;
        #(SPI_CLOCK);
            for(s = 0; s < SPI_BYTES; s = s + 1) begin
                for(b = 0; b < 8; b = b + 1) begin
                    spi_mosi = data[ (((SPI_BYTES-s)*8)-1) - b];
                    spi_clk = 1;
                    #(SPI_CLOCK);
                    spi_clk = 0;
                    #(SPI_CLOCK);
                end
                #(SPI_CLOCK);
            end
        spi_cs_N = 1;
        spi_mosi = 0;
    end
    endtask

    reg [8*SPI_BYTES-1:0] tmp;
    
    initial begin
        repeat(5) @(negedge clk);
        @(negedge clk) reset_n = 1;
        repeat(10) @(negedge clk);
        SPISendSingle(8'hc1); // not valid data since need 3 bytes
        #(SPI_CLOCK*30);
        tmp = {8'hc1, 8'hc2, 8'hc3};
        SPISendMulti(tmp);
        repeat(30) @(negedge clk);
        $stop;
    end

endmodule
