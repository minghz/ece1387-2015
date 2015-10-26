/*------------------------------------------------------------------------------
 * This code was generated by Spiral Multiplier Block Generator, www.spiral.net
 * Copyright (c) 2006, Carnegie Mellon University
 * All rights reserved.
 * The code is distributed under a BSD style license
 * (see http://www.opensource.org/licenses/bsd-license.php)
 *------------------------------------------------------------------------------ */
/* ./multBlockGen.pl 14705 -fractionalBits 0*/
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
    w1024,
    w1025,
    w32,
    w993,
    w128,
    w865,
    w13840,
    w14705;

  assign w1 = i_data0;
  assign w1024 = w1 << 10;
  assign w1025 = w1 + w1024;
  assign w128 = w1 << 7;
  assign w13840 = w865 << 4;
  assign w14705 = w865 + w13840;
  assign w32 = w1 << 5;
  assign w865 = w993 - w128;
  assign w993 = w1025 - w32;

  assign o_data0 = w14705;

  //multiplier_block area estimate = 6419.95199098235;
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
