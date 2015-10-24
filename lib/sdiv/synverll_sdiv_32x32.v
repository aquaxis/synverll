/*
 * Copyright (C)2014-2015 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 *
 * For further information please contact.
 *  URI:    http://www.aquaxis.com/
 *  E-Mail: info(at)aquaxis.com
 */
module synverll_sdiv_32x32(
	input			system_clock,
	input			system_reset,

	input			__call_sdiv_req,
	output			__call_sdiv_ready,
	output 			__call_sdiv_done,
	input [31:0]	__call_sdiv_args_0,
	input [31:0]	__call_sdiv_args_1,
	output [31:0]	__call_sdiv_result
);

reg [33:0] flag_done;

assign __call_sdiv_done		= flag_done[33];
assign __call_sdiv_ready	= 1;

always @(posedge system_clock or negedge system_reset) begin
	if(!system_reset) begin
		flag_done <= 0;
	end else begin
		flag_done[33:0] <= {flag_done[32:0], __call_sdiv_req};
	end
end

aq_div32x32 u_aq_div32x32(
	.RST_N	( system_reset			),
	.CLK	( system_clock			),
	.DINA	( __call_sdiv_args_0	),
	.DINB	( __call_sdiv_args_1	),
	.DOUT	( __call_sdiv_result	)
);

endmodule
