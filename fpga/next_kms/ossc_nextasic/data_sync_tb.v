// TODO: update edge
module test_DataSync_Fast_Slow;

	reg in_clk = 0;
	reg [3:0] in_data = 0;
	reg in_data_valid = 0;
	reg out_clk = 0;
	reg out_data_retrieved = 0;
	
	wire [3:0] out_data;
	wire out_data_valid;
	
	parameter IN_CLOCK = 10;
	parameter OUT_CLOCK = IN_CLOCK*100;

	DataSync sync(
		in_clk,
		in_data,
		in_data_valid,
		out_clk,
		out_data,
		out_data_valid,
		out_data_retrieved
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;
	
	
	initial begin
		@(posedge out_data_valid);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
	end
	
	initial begin
		#(IN_CLOCK*10);
		in_data = 4'd1;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*10);
		in_data = 4'd2; // this data will be lost
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*1000);
		
		
		in_data = 4'd3;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*1000);
		$stop;
	end
	
endmodule

// TODO: update edge
module test_DataSync_Slow_Fast;

	reg in_clk = 0;
	reg [3:0] in_data = 0;
	reg in_data_valid = 0;
	reg out_clk = 0;
	reg out_data_retrieved = 0;
	
	wire [3:0] out_data;
	wire out_data_valid;
	
	
	parameter OUT_CLOCK = 10;
	parameter IN_CLOCK = OUT_CLOCK*100;

	DataSync sync(
		in_clk,
		in_data,
		in_data_valid,
		out_clk,
		out_data,
		out_data_valid,
		out_data_retrieved
	);
	
	always #(IN_CLOCK/2) in_clk = ~in_clk;
	always #(OUT_CLOCK/2) out_clk = ~out_clk;
	
	initial begin
		@(posedge out_data_valid);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
	end
	
	initial begin
		#(IN_CLOCK*10);
		in_data = 4'd1;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		
		in_data = 4'd2; // this data will be lost
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		#(IN_CLOCK*10);
		
		in_data = 4'd3;
		@(negedge in_clk) in_data_valid = 1;
		@(negedge in_clk) in_data_valid = 0;
		
		@(posedge out_data_valid);
		@(posedge out_clk);
		@(posedge out_clk);
		@(posedge out_clk) out_data_retrieved = 1;
		@(posedge out_clk) out_data_retrieved = 0;
		
		#(IN_CLOCK*10);
		
		$stop;
	end
	
endmodule
