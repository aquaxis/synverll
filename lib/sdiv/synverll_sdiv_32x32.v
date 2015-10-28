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
	output signed [31:0]	__call_sdiv_q,
	output signed [31:0]	__call_sdiv_r
);

reg [33:0] flag_done;
reg [33:0] flag_signed;
reg [30:0] reg_args_0, reg_args_1;

wire [30:0] wire_q, wire_r;

assign __call_sdiv_done		= flag_done[33];
assign __call_sdiv_ready	= 1;

assign __call_sdiv_q = {flag_signed[33], wire_q};
assign __call_sdiv_r = {0, wire_r};

always @(posedge system_clock or negedge system_reset) begin
	if(!system_reset) begin
		flag_done <= 0;
		flag_signed <= 0;
		reg_args_0 <= 0;
		reg_args_1 <= 0;
	end else begin
		flag_done[33:0] <= {flag_done[32:0], __call_sdiv_req};
		flag_signed[33:0] <= {flag_signed[32:0], (__call_sdiv_args_0[31] ^ __call_sdiv_args_1[31])};
		reg_args_0[30:0] <= (__call_sdiv_args_0[31])?(~__call_sdiv_args_0[30:0]+1):(__call_sdiv_args_0[30:0]);
		reg_args_1[30:0] <= (__call_sdiv_args_1[31])?(~__call_sdiv_args_1[30:0]+1):(__call_sdiv_args_1[30:0]);
	end
end

aq_div31x31 u_aq_div31x31(
	.RST_N	( system_reset			),
	.CLK	( system_clock			),
	.DINA	( reg_args_0	),
	.DINB	( reg_args_1	),
	.QOUT	( wire_q			),
	.ROUT	( wire_r			)
);

endmodule
