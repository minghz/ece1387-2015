/*------------------------------------------------------------------------------
 * This code was generated by Spiral Multiplier Block Generator, www.spiral.net
 * Copyright (c) 2006, Carnegie Mellon University
 * All rights reserved.
 * The code is distributed under a BSD style license
 * (see http://www.opensource.org/licenses/bsd-license.php)
 *------------------------------------------------------------------------------ */
/* ./multBlockGen.pl 12554 -fractionalBits 0*/
module multiplier_block (
    i_data0,
    o_data0
);

  // Port mode declarations:
  input   [31:0] i_data0;
  output  [31:0]
    o_data0;

  //Multipliers:

  wire [31:0]
    w1,
    w32,
    w33,
    w4224,
    w4225,
    w512,
    w513,
    w2052,
    w6277,
    w12554;

  assign w1 = i_data0;
  assign w12554 = w6277 << 1;
  assign w2052 = w513 << 2;
  assign w32 = w1 << 5;
  assign w33 = w1 + w32;
  assign w4224 = w33 << 7;
  assign w4225 = w1 + w4224;
  assign w512 = w1 << 9;
  assign w513 = w1 + w512;
  assign w6277 = w4225 + w2052;

  assign o_data0 = w12554;

  //multiplier_block area estimate = 6266.93775068823;
endmodule //multiplier_block

module surround_with_regs(
	i_data0,
	o_data0,
	clk
);

	// Port mode declarations:
	input   [31:0] i_data0;
	output  [31:0] o_data0;
	reg  [31:0] o_data0;
	input clk;

	reg [31:0] i_data0_reg;
	wire [30:0] o_data0_from_mult;

	always @(posedge clk) begin
		i_data0_reg <= i_data0;
		o_data0 <= o_data0_from_mult;
	end

	multiplier_block mult_blk(
		.i_data0(i_data0_reg),
		.o_data0(o_data0_from_mult)
	);

endmodule
